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

#include "IOrchestrionPlayer.h"
#include "IOrchestrionSequencer.h"
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <async/notification.h>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>

namespace dgk
{
class AutomaticOrchestrionPlayer : public IOrchestrionPlayer,
                                   public muse::async::Asyncable,
                                   public muse::actions::Actionable,
                                   public muse::Injectable
{
  muse::Inject<mu::playback::IPlaybackController> playbackController;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;

public:
  AutomaticOrchestrionPlayer(IOrchestrionSequencer &sequencer);

  // IOrchestrionPlayer
  bool IsPlaying() const override { return m_playing; }
  muse::async::Notification PlayingChanged() const override
  {
    return m_playingChanged;
  }

private:
  void TogglePlay();
  void Stop();
  void ScheduleNext();
  void FireAndContinue(const NextAutoPlayEvents &events);
  int TicksToMilliseconds(int ticks) const;

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

  muse::async::Notification m_playingChanged;
};
} // namespace dgk
