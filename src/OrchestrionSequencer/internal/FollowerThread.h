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

#include "IVisibleScore.h"
#include "OrchestrionCommon/OfflineRendering.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "PerformanceFollower.h"
#include "PlayThread.h"

#include <async/asyncable.h>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace dgk
{
/**
 * A PerformanceFollower on a thread of its own: following a note costs time
 * in proportion to the score's length, which the thread playing the notes
 * shouldn't wait for. Used from the play thread (see PlayThread), which
 * `onFollowed` is called on; created and destroyed on the main thread, where
 * it hears of the part of the score in view.
 */
class FollowerThread : public muse::async::Asyncable, public dgk::Injectable
{
  dgk::Inject<IVisibleScore> visibleScore{this};

public:
  /**
   * What following a note found: where the performer is, when the note
   * arrived, and whether its pitch is one of the notes there.
   */
  struct Followed
  {
    int tick = 0;
    double ms = 0.0;
    int pitch = 0;
    bool matched = false;
  };

  /** `onFollowed` is told of every note followed, on `playThread`. */
  FollowerThread(PlayThread &playThread,
                 std::function<void(const Followed &)> onFollowed);
  ~FollowerThread();

  /**
   * Stops following, for good: what was found and not told yet is dropped,
   * and one being told is waited for. Whatever thread is told is then let be.
   */
  void Stop();

  /**
   * Follows these events from now on — nothing if there are none — at first
   * at that tempo. What was under way is dropped.
   */
  void Follow(std::vector<ScoreEvent>, double nominalTicksPerMs);

  /** The events followed; none if nothing is. */
  const std::vector<ScoreEvent> *Events() const;

  /**
   * Expects the next note at `event` (see PerformanceFollower::Reset). What
   * was under way is dropped.
   */
  void Reset(size_t event);

  /**
   * A note-on at time `ms`, the performance going at `ticksPerMs` if it is
   * known otherwise (see PerformanceFollower::SetTicksPerMs).
   */
  void OnNoteOn(int pitch, double ms, std::optional<double> ticksPerMs);

private:
  struct Task
  {
    enum class Kind
    {
      follow,
      reset,
      note,
      visible
    };
    Kind kind = Kind::note;
    unsigned generation = 0;
    std::shared_ptr<PerformanceFollower> follower;
    size_t event = 0;
    int pitch = 0;
    double ms = 0.0;
    std::optional<double> ticksPerMs;
    std::pair<int, int> visible;
  };

  /**
   * Tells the follower thread what the performer sees. On the main thread.
   */
  void PostVisibleTicks();
  void Post(Task);
  void Run();
  /** The follower thread's, or the play thread's if m_inline. */
  void Handle(Task);

  PlayThread &m_playThread;
  const std::function<void(const Followed &)> m_onFollowed;
  /**
   * The follower's events are read here, which never change; the rest of it
   * is the follower thread's.
   */
  std::shared_ptr<PerformanceFollower> m_follower;
  /**
   * Counts the restarts — Follow and Reset — so that what the follower thread
   * found before the last one is dropped.
   */
  unsigned m_generation = 0;

  /**
   * Whether it still is: what it found is told on the play thread, maybe
   * after it is destroyed on the main thread, which waits for one being told.
   */
  struct Life
  {
    std::mutex mutex;
    bool alive = true;
  };
  const std::shared_ptr<Life> m_life = std::make_shared<Life>();

  /**
   * Rendering offline, the tasks are handled as they are posted, on the play
   * thread (the main thread then), not on a thread of their own: the
   * follower's findings come at the virtual time of the notes followed, the
   * same from one render to the next.
   */
  const bool m_inline = !offlineRendering.audioPath.empty();
  /** The follower the tasks are handled with (see Handle). */
  std::shared_ptr<PerformanceFollower> m_handledFollower;

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::deque<Task> m_tasks;
  /** The part of the score in view, given to each new follower. */
  std::optional<std::pair<int, int>> m_visible;
  bool m_finished = false;
  std::thread m_thread;
};
} // namespace dgk
