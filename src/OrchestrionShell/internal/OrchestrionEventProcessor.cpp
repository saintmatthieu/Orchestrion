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
#include "OrchestrionEventProcessor.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"

namespace dgk
{
muse::midi::Event
OrchestrionEventProcessor::ToMuseMidiEvent(const NoteEvent &dgkEvent) const
{
  muse::midi::Event museEvent(dgkEvent.type == dgk::NoteEventType::noteOn
                                  ? muse::midi::Event::Opcode::NoteOn
                                  : muse::midi::Event::Opcode::NoteOff,
                              muse::midi::Event::MessageType::ChannelVoice10);

  const auto channel = mapper()->channelForTrack(
      dgkEvent.track, ITrackChannelMapper::Policy::oneChannelPerInstrument);
  museEvent.setChannel(channel);
  museEvent.setNote(dgkEvent.pitch);
  museEvent.setVelocity(std::clamp<uint16_t>(dgkEvent.velocity * 128, 0, 127));
  return museEvent;
}

muse::midi::Event
OrchestrionEventProcessor::ToMuseMidiEvent(const PedalEvent &pedalEvent) const
{
  muse::midi::Event museEvent(muse::midi::Event::Opcode::ControlChange,
                              muse::midi::Event::MessageType::ChannelVoice10);
  const auto channels = mapper()->channelsForInstrument(
      pedalEvent.instrument,
      ITrackChannelMapper::Policy::oneChannelPerInstrument);
  assert(channels.size() == 1);
  museEvent.setChannel(channels[0]);
  museEvent.setIndex(0x40);
  museEvent.setData(pedalEvent.on ? 127 : 0);
  return museEvent;
}

void OrchestrionEventProcessor::init()
{
  if (const auto sequencer = orchestrion()->sequencer())
    setupCallback(*sequencer);
  orchestrion()->sequencerChanged().onNotify(
      this,
      [this]
      {
        if (auto sequencer = orchestrion()->sequencer())
          setupCallback(*sequencer);
      });
}

void OrchestrionEventProcessor::setupCallback(IOrchestrionSequencer &sequencer)
{
  sequencer.OutputEvent().onReceive(this, [this](EventVariant event)
                                    { onOrchestrionEvent(event); });
}

void OrchestrionEventProcessor::onOrchestrionEvent(EventVariant event)
{
  if (std::holds_alternative<NoteEvents>(event))
  {
    const auto &events = std::get<NoteEvents>(event);
    std::for_each(events.begin(), events.end(), [&](const NoteEvent &event)
                  { midiOutPort()->sendEvent(ToMuseMidiEvent(event)); });
  }
  else if (std::holds_alternative<PedalEvent>(event))
  {
    const auto &pedalEvent = std::get<PedalEvent>(event);
    midiOutPort()->sendEvent(ToMuseMidiEvent(pedalEvent));
  }
}
} // namespace dgk