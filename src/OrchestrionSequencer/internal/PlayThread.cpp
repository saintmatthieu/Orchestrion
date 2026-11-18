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
#include "PlayThread.h"
#include "OrchestrionCommon/ThreadPriority.h"

#include <QMetaObject>
#include <QObject>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace dgk
{
class PlayThread::Impl
{
public:
  virtual ~Impl() = default;
  virtual void PostAt(std::optional<Clock::time_point>, Task) = 0;
  virtual bool IsCurrent() const = 0;
};

/**
 * A thread of its own. The tasks wait in one map, ordered by when they are
 * due and then by when they were posted; untimed ones are due at the clock's
 * dawn, so they come first, in order.
 */
class PlayThread::OwnThread : public PlayThread::Impl
{
public:
  OwnThread() : m_thread{[this] { Run(); }} {}

  ~OwnThread() override
  {
    {
      std::scoped_lock lock{m_mutex};
      m_stopping = true;
    }
    m_cv.notify_one();
    m_thread.join();
  }

  void PostAt(std::optional<Clock::time_point> time, Task task) override
  {
    {
      std::scoped_lock lock{m_mutex};
      m_tasks.emplace(Key{time.value_or(Clock::time_point::min()), m_serial++},
                      std::move(task));
    }
    m_cv.notify_one();
  }

  bool IsCurrent() const override
  {
    return std::this_thread::get_id() == m_thread.get_id();
  }

private:
  using Key = std::pair<Clock::time_point, std::uint64_t>;

  void Run()
  {
    raiseTimedThreadPriority();
    std::unique_lock lock{m_mutex};
    while (true)
    {
      if (m_stopping)
        return;
      if (m_tasks.empty())
      {
        m_cv.wait(lock);
        continue;
      }
      const auto first = m_tasks.begin();
      const Clock::time_point due = first->first.first;
      if (due > Clock::now())
      {
        // Until it is due, or something else is posted, maybe due earlier.
        m_cv.wait_until(lock, due);
        continue;
      }
      Task task = std::move(first->second);
      m_tasks.erase(first);
      lock.unlock();
      task();
      // Destroyed out of the lock too: it may hold what posts.
      task = nullptr;
      lock.lock();
    }
  }

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::map<Key, Task> m_tasks;
  std::uint64_t m_serial = 0;
  bool m_stopping = false;
  // Last: started once the rest is ready.
  std::thread m_thread;
};

/**
 * The main thread, offline rendering: Qt's queued calls and timers, which go
 * by the virtual clock there.
 */
class PlayThread::MainThread : public PlayThread::Impl
{
public:
  MainThread() : m_thread{QThread::currentThread()} {}

  void PostAt(std::optional<Clock::time_point> time, Task task) override
  {
    if (!time)
    {
      QMetaObject::invokeMethod(&m_context, std::move(task),
                                Qt::QueuedConnection);
      return;
    }
    // The delay is reckoned on the main thread: another doesn't read the
    // virtual clock.
    auto arm = [this, time = *time, task = std::move(task)]
    {
      const auto delay = std::max(Clock::duration::zero(), time - Clock::now());
      QTimer::singleShot(
          std::chrono::duration_cast<std::chrono::nanoseconds>(delay),
          Qt::PreciseTimer, &m_context, task);
    };
    if (IsCurrent())
      arm();
    else
      QMetaObject::invokeMethod(&m_context, std::move(arm),
                                Qt::QueuedConnection);
  }

  bool IsCurrent() const override
  {
    return QThread::currentThread() == m_thread;
  }

private:
  QThread *const m_thread;
  // Its tasks are dropped with it.
  QObject m_context;
};

PlayThread::PlayThread(bool onMainThread)
    : m_impl{onMainThread
                 ? std::unique_ptr<Impl>{std::make_unique<MainThread>()}
                 : std::make_unique<OwnThread>()}
{
}

PlayThread::~PlayThread() = default;

void PlayThread::Post(Task task)
{
  m_impl->PostAt(std::nullopt, std::move(task));
}

void PlayThread::PostAt(Clock::time_point time, Task task)
{
  m_impl->PostAt(time, std::move(task));
}

bool PlayThread::IsCurrent() const { return m_impl->IsCurrent(); }

void PlayThread::Wait(Task task)
{
  if (IsCurrent())
  {
    task();
    return;
  }
  std::promise<void> done;
  std::future<void> future = done.get_future();
  Post(
      [&]
      {
        task();
        done.set_value();
      });
  future.wait();
}
} // namespace dgk
