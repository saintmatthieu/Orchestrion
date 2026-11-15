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

#include "IOrchestrionPlayer.h"
#include "internal/OrchestrionSequencer.h"
#include "internal/PlayThread.h"

#include <async/asyncable.h>
#include <map>
#include <memory>
#include <optional>

namespace dgk
{
/**
 * Plays the Follow staves (see IStaffModes): strikes and releases their
 * chords as the performance's estimate (see
 * IOrchestrionSequencer::PerformedTempoNow) reaches them, so that they follow
 * the performer — or the automatic player, standing in for them. When the
 * performer stops, the estimate coasts to a halt and the Follow staves halt
 * with it.
 *
 * Created and destroyed with the sequencer and the automatic player, on the
 * play thread, where it lives (see ../threading.md).
 */
class FollowPlayer : public muse::async::Asyncable
{
public:
  FollowPlayer(OrchestrionSequencer &, const IOrchestrionPlayer &,
               PlayThread &);

private:
  struct Targets
  {
    /** When the chord sounding is to be released, ticks with repeats. */
    std::optional<double> offTick;
    /** When the next chord is to be struck. */
    std::optional<double> onTick;
  };

  bool IsFollowStaff(int staff) const;
  /**
   * Refreshes the targets of the Follow staves' voices this batch brings.
   * Batches only carry the tracks that changed: the per-voice ledger keeps
   * the others'.
   */
  void OnTransitions(const std::map<TrackIndex, ChordTransition> &);
  /** Each Follow staff's earliest release and strike across its voices. */
  void Aggregate();
  /** Fires what the estimate has reached. */
  void Poll();
  /**
   * The tempo is known from now on — at first, or again after a pause or a
   * stop: the Follow staves join from where the performer is, leaving out
   * what they had due before, rather than rattling it off to catch up.
   * Returns whether any was moved on.
   */
  bool Join(double performedTick);
  void Fire(int staff, bool noteOn);
  /** Polls in a while, and so on, while m_polling. */
  void SchedulePoll();
  /** Runs the task on the play thread, unless it is gone by then. */
  void Post(std::function<void()>,
            std::optional<PlayThread::Clock::time_point> time = std::nullopt);

  OrchestrionSequencer &m_sequencer;
  const IOrchestrionPlayer &m_player;
  PlayThread &m_thread;
  std::map<TrackIndex, Targets> m_trackTargets;
  std::map<int /*staff*/, Targets> m_staffTargets;
  /** There was no estimate to follow last time the targets were polled. */
  bool m_awaitsTempo = true;
  /**
   * Whether it polls the targets against the estimate, which it does while
   * there is anything to fire. A driver of its own: the performer may stop,
   * the estimate then coasting to a halt between gestures.
   */
  bool m_polling = false;
  /** Tells the polls scheduled from those superseded. */
  unsigned m_pollGeneration = 0;
  /** Its tasks posted are dropped once it is gone. */
  const std::shared_ptr<int> m_alive = std::make_shared<int>(0);
};
} // namespace dgk
