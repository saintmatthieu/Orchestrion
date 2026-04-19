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
                                              ScheduleNext();
                                            });

  playbackController()->isPlayingChanged().onNotify(
      this,
      [this]
      {
        m_playing = playbackController()->isPlaying();
        if (m_playing)
        {
          ++m_generation;
          ScheduleNext();
        }
      });
}

void AutomaticOrchestrionPlayer::ScheduleNext()
{
  const auto next = m_sequencer.WhatToPlayNext();
  if (!next)
  {
    dispatcher()->dispatch("stop");
    m_playing = false;
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
  if (events.leftHandEvent)
    m_sequencer.OnInputEvent(*events.leftHandEvent, leftHandPitch,
                             std::nullopt);
  if (events.rightHandEvent)
    m_sequencer.OnInputEvent(*events.rightHandEvent, rightHandPitch,
                             std::nullopt);
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
