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
FollowerThread::FollowerThread(PlayThread &playThread,
                               std::function<void(const Followed &)> onFollowed)
    : m_playThread{playThread}, m_onFollowed{std::move(onFollowed)}
{
  visibleScore()->visibleTicksChanged().onNotify(this, [this]
                                                 { PostVisibleTicks(); });
  PostVisibleTicks();
  m_thread = std::thread{[this] { Run(); }};
}

FollowerThread::~FollowerThread() { Stop(); }

void FollowerThread::Stop()
{
  {
    std::scoped_lock lock{m_mutex};
    m_finished = true;
  }
  m_cv.notify_one();
  if (m_thread.joinable())
    m_thread.join();
  // What it found and has yet to tell is dropped; one being told is waited
  // for.
  std::scoped_lock lock{m_life->mutex};
  m_life->alive = false;
}

void FollowerThread::Follow(std::vector<ScoreEvent> events,
                            double nominalTicksPerMs)
{
  m_follower = events.empty() ? nullptr
                              : std::make_shared<PerformanceFollower>(
                                    std::move(events), nominalTicksPerMs);
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
  if (!visible)
    return;
  {
    std::scoped_lock lock{m_mutex};
    m_visible = visible;
  }
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
    {
      follower = std::move(task.follower);
      std::optional<std::pair<int, int>> visible;
      {
        std::scoped_lock lock{m_mutex};
        visible = m_visible;
      }
      if (follower && visible)
        follower->SetVisibleTicks(visible->first, visible->second);
      break;
    }
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
      const auto has = [&](const std::vector<int> &pitches)
      {
        return std::find(pitches.begin(), pitches.end(), task.pitch) !=
               pitches.end();
      };
      const bool matched = has(event.pitches) || has(event.trillPitches);
      m_playThread.Post(
          [this, life = m_life, generation = task.generation,
           followed = Followed{event.tick, task.ms, task.pitch, matched}]
          {
            std::scoped_lock lock{life->mutex};
            if (life->alive && generation == m_generation)
              m_onFollowed(followed);
          });
      break;
    }
    }
  }
}
} // namespace dgk
