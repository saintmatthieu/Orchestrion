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
#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace dgk
{
/**
 * A thread for gesture input devices to be listened to on, rather than on the
 * main thread, which may be busy (see
 * OrchestrionSequencer/threading.md): what is subscribed to from it is
 * received there. kors has no way of waking a thread when a message is
 * queued for it, so it looks every millisecond, as the main thread does.
 *
 * The main thread never waits for it but to create and destroy what lives on
 * it, so what lives there may send to the main thread.
 */
class InputThread
{
public:
  InputThread();
  /** Runs what was posted; drops what kors has yet to deliver. */
  ~InputThread();

  InputThread(const InputThread &) = delete;
  InputThread &operator=(const InputThread &) = delete;

  /** Runs the task on the thread and returns once it has run. */
  void Wait(std::function<void()> task);

private:
  void Run();

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::deque<std::function<void()>> m_tasks;
  bool m_stopping = false;
  // Last: started once the rest is ready.
  std::thread m_thread;
};
} // namespace dgk
