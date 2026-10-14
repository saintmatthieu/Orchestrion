/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "FollowerThread.h"

#include <algorithm>

namespace dgk
{
FollowerThread::FollowerThread(std::function<void(const Followed &)> onFollowed)
    : m_onFollowed{std::move(onFollowed)}
{
  m_results.onReceive(this,
                      [this](const Result &result)
                      {
                        if (result.generation == m_generation)
                          m_onFollowed(result.followed);
                      });
  visibleScore()->visibleTicksChanged().onNotify(this,
                                                 [this] { PostVisibleTicks(); });
  m_thread = std::thread{[this] { Run(); }};
}

FollowerThread::~FollowerThread()
{
  {
    std::scoped_lock lock{m_mutex};
    m_finished = true;
  }
  m_cv.notify_one();
  m_thread.join();
}

void FollowerThread::Follow(std::vector<ScoreEvent> events,
                            double nominalTicksPerMs)
{
  m_follower = events.empty() ? nullptr
                              : std::make_shared<PerformanceFollower>(
                                    std::move(events), nominalTicksPerMs);
  // Not the follower thread's yet.
  if (const auto visible = visibleScore()->visibleTicks(); m_follower && visible)
    m_follower->SetVisibleTicks(visible->first, visible->second);
  Task task;
  task.kind = Task::Kind::follow;
  task.generation = ++m_generation;
  task.follower = m_follower;
  Post(std::move(task));
}

const std::vector<ScoreEvent> *FollowerThread::Events() const
{
  return m_follower ? &m_follower->Events() : nullptr;
}

void FollowerThread::Reset(size_t event)
{
  Task task;
  task.kind = Task::Kind::reset;
  task.generation = ++m_generation;
  task.event = event;
  Post(std::move(task));
}

void FollowerThread::OnNoteOn(int pitch, double ms,
                              std::optional<double> ticksPerMs)
{
  if (!m_follower)
    return;
  Task task;
  task.generation = m_generation;
  task.pitch = pitch;
  task.ms = ms;
  task.ticksPerMs = ticksPerMs;
  Post(std::move(task));
}

void FollowerThread::PostVisibleTicks()
{
  const auto visible = visibleScore()->visibleTicks();
  if (!m_follower || !visible)
    return;
  Task task;
  task.kind = Task::Kind::visible;
  task.visible = *visible;
  Post(std::move(task));
}

void FollowerThread::Post(Task task)
{
  {
    std::scoped_lock lock{m_mutex};
    m_tasks.push_back(std::move(task));
  }
  m_cv.notify_one();
}

void FollowerThread::Run()
{
  std::shared_ptr<PerformanceFollower> follower;
  while (true)
  {
    Task task;
    {
      std::unique_lock lock{m_mutex};
      m_cv.wait(lock, [this] { return m_finished || !m_tasks.empty(); });
      if (m_finished)
        return;
      task = std::move(m_tasks.front());
      m_tasks.pop_front();
    }
    switch (task.kind)
    {
    case Task::Kind::follow:
      follower = std::move(task.follower);
      break;
    case Task::Kind::reset:
      if (follower)
        follower->Reset(task.event);
      break;
    case Task::Kind::visible:
      if (follower)
        follower->SetVisibleTicks(task.visible.first, task.visible.second);
      break;
    case Task::Kind::note:
    {
      if (!follower)
        break;
      if (task.ticksPerMs)
        follower->SetTicksPerMs(*task.ticksPerMs);
      follower->OnNoteOn(task.pitch, task.ms);
      const ScoreEvent &event = follower->Events()[follower->FollowedEvent()];
      const bool matched = std::find(event.pitches.begin(), event.pitches.end(),
                                     task.pitch) != event.pitches.end();
      m_results.send(Result{
          task.generation, Followed{event.tick, task.ms, task.pitch, matched}});
      break;
    }
    }
  }
}
} // namespace dgk
