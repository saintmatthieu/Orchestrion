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

#include "IOrchestrionSequencer.h"
#include <QElapsedTimer>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>

namespace dgk
{
class AutomaticOrchestrionPlayer : public muse::async::Asyncable,
                                   public muse::Injectable
{
  muse::Inject<mu::playback::IPlaybackController> playbackController;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;

public:
  AutomaticOrchestrionPlayer(IOrchestrionSequencer &sequencer);

  //! While set, play replays this recorded take — rewind to its start tick,
  //! re-inject its input events at their recorded times — instead of the
  //! metronomic playback.
  void SetReplayTake(std::optional<ReplayTake> take);
  bool IsReplaying() const { return m_replayActive; }

private:
  void ScheduleNext();
  void FireAndContinue(const NextAutoPlayEvents &events);
  int TicksToMilliseconds(int ticks) const;
  void StartReplay();
  void ScheduleReplayNext();
  void FireReplayEvent();

  IOrchestrionSequencer &m_sequencer;
  bool m_playing = false;
  // Used to cancel previously scheduled calls to FireAndContinue when the
  // position changes e.g. by the user pressing left/right during playback.
  int m_generation = 0;
  // True while our own OnInputEvent calls are on the stack. A position jump
  // they cause (e.g. the loop wrap-around) must not re-enter ScheduleNext
  // from the AboutToJumpPosition notification — the sequencer state is not
  // settled yet, and FireAndContinue reschedules after they return anyway.
  bool m_firingInputEvents = false;

  std::optional<ReplayTake> m_replayTake;
  bool m_replayActive = false;
  // True while the replay performs its own rewind to the take's start, so
  // that jump isn't taken for the user navigating away.
  bool m_selfJump = false;
  std::size_t m_replayIndex = 0;
  QElapsedTimer m_replayClock;
};
} // namespace dgk
