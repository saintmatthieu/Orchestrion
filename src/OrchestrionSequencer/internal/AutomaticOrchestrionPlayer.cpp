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

namespace dgk
{
namespace
{
constexpr int rightHandPitch = 60;
constexpr int leftHandPitch = 59;
} // namespace

AutomaticOrchestrionPlayer::AutomaticOrchestrionPlayer(
    IOrchestrionSequencer &sequencer)
    : m_sequencer{sequencer}
{
  Advance();

  sequencer.AboutToJumpPosition().onReceive(this,
                                            [this](int tick)
                                            {
                                              m_targetTick = tick;
                                              m_needsRefetch = true;
                                            });

  playbackController()->isPlayingChanged().onNotify(
      this,
      [this]
      {
        if (playbackController()->isPlaying())
        {
          m_done = false;
          m_needsRefetch = true;
        }
      });

  playbackController()->currentPlaybackPositionChanged().onReceive(
      this,
      [this](muse::audio::secs_t, muse::midi::tick_t tick)
      {
        if (!playbackController()->isPlaying() || m_done)
          return;
        const int t = static_cast<int>(tick);
        if (m_needsRefetch)
        {
          m_needsRefetch = false;
          Advance();
        }
        while (m_next && t >= m_targetTick)
        {
          if (m_next->leftHandEvent)
            m_sequencer.OnInputEvent(*m_next->leftHandEvent, leftHandPitch,
                                     std::nullopt);
          if (m_next->rightHandEvent)
            m_sequencer.OnInputEvent(*m_next->rightHandEvent, rightHandPitch,
                                     std::nullopt);
          if (m_needsRefetch)
          {
            m_done = true;
            return;
          }
          Advance();
        }
      });
}

void AutomaticOrchestrionPlayer::Advance()
{
  m_next = m_sequencer.WhatToPlayNext();
  if (m_next)
    m_targetTick += m_next->deltaTicks;
  else
  {
    m_done = true;
    dispatcher()->dispatch("stop");
  }
}
} // namespace dgk
