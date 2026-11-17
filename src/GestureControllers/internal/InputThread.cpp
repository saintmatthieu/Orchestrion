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
#include "InputThread.h"

#include <async/processevents.h>
#include <chrono>
#include <future>

#include "OrchestrionCommon/TimingThread.h" // last: <windows.h>

namespace dgk
{
InputThread::InputThread() : m_thread{[this] { Run(); }} {}

InputThread::~InputThread()
{
  {
    std::scoped_lock lock{m_mutex};
    m_stopping = true;
  }
  m_cv.notify_one();
  m_thread.join();
}

void InputThread::Wait(std::function<void()> task)
{
  if (std::this_thread::get_id() == m_thread.get_id())
  {
    task();
    return;
  }
  std::promise<void> done;
  std::future<void> future = done.get_future();
  {
    std::scoped_lock lock{m_mutex};
    m_tasks.push_back(
        [&]
        {
          task();
          done.set_value();
        });
  }
  m_cv.notify_one();
  future.wait();
}

void InputThread::Run()
{
  SetUpTimingThread("orch-midi-in");
  const std::thread::id self = std::this_thread::get_id();
  std::unique_lock lock{m_mutex};
  while (!m_stopping)
  {
    while (!m_tasks.empty())
    {
      auto task = std::move(m_tasks.front());
      m_tasks.pop_front();
      lock.unlock();
      task();
      lock.lock();
    }
    lock.unlock();
    muse::async::processMessages(self);
    lock.lock();
    m_cv.wait_for(lock, std::chrono::milliseconds{1},
                  [this] { return m_stopping || !m_tasks.empty(); });
  }
}
} // namespace dgk
