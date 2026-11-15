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

#include <chrono>
#include <functional>
#include <memory>

namespace dgk
{
/**
 * The thread the sequencer plays on (see ../threading.md): tasks run one at a
 * time, in the order posted, or once their time has come, on the steady
 * clock. It never waits for the main thread, so the main thread may wait for
 * it (Wait).
 *
 * Offline rendering, time is virtual on the main thread only: the play thread
 * is then the main thread itself (`onMainThread`), its tasks queued Qt calls
 * and its timed tasks Qt timers.
 */
class PlayThread
{
public:
  using Clock = std::chrono::steady_clock;
  using Task = std::function<void()>;

  /**
   * A thread of its own, or, `onMainThread`, the main thread, which must
   * then be the one creating it.
   */
  explicit PlayThread(bool onMainThread = false);
  /**
   * Waits for the task under way, if any; those not run yet are dropped.
   */
  ~PlayThread();

  PlayThread(const PlayThread &) = delete;
  PlayThread &operator=(const PlayThread &) = delete;

  /** Runs the task after those posted before it. */
  void Post(Task);

  /**
   * Runs the task at `time`, or as soon as possible if that is past. Tasks
   * due at the same time run in the order posted; a task posted with Post
   * doesn't wait for timed ones due earlier.
   */
  void PostAt(Clock::time_point time, Task);

  /** Whether the caller is on the play thread. */
  bool IsCurrent() const;

  /**
   * Runs the task and returns once it has run — right away, if called on the
   * play thread. Never to be called from a thread the play thread may be
   * waiting for (it waits for none).
   */
  void Wait(Task);

private:
  class Impl;
  class OwnThread;
  class MainThread;
  std::unique_ptr<Impl> m_impl;
};
} // namespace dgk
