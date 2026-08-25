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
#include "AutomaticOrchestrionPlayer.h"
#include "MuseScoreShell/OrchestrionActionIds.h"
#include <QTimer>

namespace dgk
{
namespace
{
constexpr int rightHandPitch = 60;
constexpr int leftHandPitch = 59;
constexpr int ticksPerQuarterNote = 480;
} // namespace

AutomaticOrchestrionPlayer::AutomaticOrchestrionPlayer(
    IOrchestrionSequencer &sequencer)
    : m_sequencer{sequencer}
{
  sequencer.AboutToJumpPosition().onReceive(this,
                                            [this](int /*tick*/)
                                            {
                                              ++m_generation;
                                              if (!m_firingInputEvents)
                                                ScheduleNext();
                                            });

  // Orchestrion owns its playing state, driven by its own transport actions
  // (see OrchestrionActionIds.h). MuseScore's "play"/"stop" are neither
  // handled nor dispatched: its transport machinery was built for
  // rendered-track playback — a playhead running over the (now silent)
  // tracks, auto-stopping at *its* score end, stopped by *its* code (e.g.
  // the preferences dialog) — while Orchestrion playback is just scheduled
  // gesture events. With nothing dispatching MuseScore's ids, its transport
  // simply never runs.
  dispatcher()->reg(this, actionIds::playbackToggle, [this] { TogglePlay(); });
  dispatcher()->reg(this, actionIds::playbackStop, [this] { Stop(); });
}

void AutomaticOrchestrionPlayer::TogglePlay()
{
  if (m_playing)
  {
    Stop();
    return;
  }
  m_playing = true;
  ++m_generation;
  m_playingChanged.notify();
  ScheduleNext();
}

void AutomaticOrchestrionPlayer::Stop()
{
  if (!m_playing)
    return;
  ++m_generation; // cancels all scheduled events
  m_playing = false;
  m_playingChanged.notify();
}

void AutomaticOrchestrionPlayer::ScheduleNext()
{
  if (!m_playing)
    return;

  const auto next = m_sequencer.WhatToPlayNext();
  if (!next)
  {
    dispatcher()->dispatch(actionIds::playbackStop);
    return;
  }

  if (next->deltaTicks > 0)
  {
    const int gen = m_generation;
    QTimer::singleShot(TicksToMilliseconds(next->deltaTicks),
                       [this, events = *next, gen]
                       {
                         if (gen == m_generation)
                           FireAndContinue(events);
                       });
  }
  else
    FireAndContinue(*next);
}

void AutomaticOrchestrionPlayer::FireAndContinue(
    const NextAutoPlayEvents &events)
{
  if (!m_playing)
    return;
  m_firingInputEvents = true;
  if (events.leftHandEvent)
    m_sequencer.OnInputEvent(*events.leftHandEvent, leftHandPitch,
                             std::nullopt);
  if (events.rightHandEvent)
    m_sequencer.OnInputEvent(*events.rightHandEvent, rightHandPitch,
                             std::nullopt);
  m_firingInputEvents = false;
  ScheduleNext();
}

int AutomaticOrchestrionPlayer::TicksToMilliseconds(int ticks) const
{
  const double bpm = playbackController()->currentTempo().valueBpm;
  const double multiplier = playbackController()->tempoMultiplier();
  if (bpm <= 0 || multiplier <= 0)
    return 0;
  return static_cast<int>(ticks * 60000.0 /
                          (bpm * ticksPerQuarterNote * multiplier));
}
} // namespace dgk
