/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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

#include "IOrchestrionSequencer.h" // ReplayTake
#include "RecordingTimeline.h"
#include <async/channel.h>
#include <async/notification.h>
#include <chrono>
#include <memory>
#include <optional>

namespace dgk
{
//! The automatic player: machine-driven gesture events — metronomic
//! playback of the score, or the replay of a recorded take. Owns
//! Orchestrion's playing state, driven by the orchestrion-play/-stop
//! actions and deliberately independent of MuseScore's transport (whose
//! playhead runs over the silent notation tracks and auto-stops at *its*
//! score end — Orchestrion playback is just scheduled gesture events).
class IOrchestrionPlayer
{
public:
  virtual ~IOrchestrionPlayer() = default;

  //! While set, play replays this recorded take — the machine re-performs
  //! it, timing, dynamics and all — instead of the metronomic playback.
  //! Cleared when a new take begins.
  virtual void SetReplayTake(std::optional<ReplayTake> take) = 0;
  //! Whether such a replay is running right now. Its re-injected events
  //! must not be judged as a new performance.
  virtual bool IsReplaying() const = 0;

  //! Whether the player is producing events (metronomic playback or a take
  //! replay).
  virtual bool IsPlaying() const = 0;
  virtual muse::async::Notification PlayingChanged() const = 0;

  /**
   * Plays along with a recording, silently, while it plays: the score moves
   * on as the recording plays it, by `timeline`. It counts as playing then.
   * Null stops following.
   */
  virtual void
  FollowRecording(std::shared_ptr<const RecordingTimeline> timeline) = 0;
  /**
   * Where the recording followed is: `seconds` into it at `when`, going at
   * `rate` if `playing`. Told as often as it is known, a few times a second.
   */
  virtual void SetRecordingClock(double seconds,
                                 std::chrono::steady_clock::time_point when,
                                 double rate, bool playing) = 0;
  /**
   * Where the recording is to go, in its seconds: the user moved the score
   * (clicked it, say) while it was being followed.
   */
  virtual muse::async::Channel<double> RecordingSeekRequested() const = 0;
  /** Whether it is playing along with a recording (see FollowRecording). */
  virtual bool IsFollowing() const = 0;
};

using IOrchestrionPlayerPtr = std::shared_ptr<IOrchestrionPlayer>;
} // namespace dgk
