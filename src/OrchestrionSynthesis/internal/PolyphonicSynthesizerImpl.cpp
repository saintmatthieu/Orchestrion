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
#include "PolyphonicSynthesizerImpl.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"

namespace dgk
{
void PolyphonicSynthesizerImpl::Initialize()
{
  Setup();
  orchestrion()->sequencerChanged().onNotify(this, [this] { Setup(); });
}

void PolyphonicSynthesizerImpl::Setup()
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  m_voices = sequencer->GetAllVoices();
  onVoicesReset();
  sequencer->AboutToJumpPosition().onReceive(this,
                                             [this](auto) { doAllNotesOff(); });
  // Stopping playback must silence like a jump does: the note-offs the
  // sequencer sends aren't enough, the synthesizer keeps ringing.
  orchestrion()->player()->PlayingChanged().onNotify(
      this,
      [this]
      {
        if (!orchestrion()->player()->IsPlaying())
          doAllNotesOff();
      });
}

int PolyphonicSynthesizerImpl::GetChannel(const TrackIndex &voice) const
{
  return std::distance(m_voices.begin(),
                       std::find(m_voices.begin(), m_voices.end(), voice));
}

} // namespace dgk