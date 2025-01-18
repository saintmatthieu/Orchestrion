#include "OrchestrionEventProcessor.h"
#include "Orchestrion/IOrchestrionSequencer.h"

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