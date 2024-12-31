#pragma once

#include "Orchestrion/IOrchestrion.h"
#include "Orchestrion/OrchestrionTypes.h"
#include "OrchestrionSynthesis/ITrackChannelMapper.h"
#include <async/asyncable.h>
#include <midi/imidioutport.h>
#include <modularity/ioc.h>

namespace dgk
{
class IOrchestrionSequencer;
class OrchestrionEventProcessor : public muse::Injectable,
                                  public muse::async::Asyncable
{
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort;
  muse::Inject<ITrackChannelMapper> mapper;

public:
  OrchestrionEventProcessor() = default;

  void init();

private:
  void setupCallback(IOrchestrionSequencer &sequencer);
  void onOrchestrionEvent(InstrumentIndex, EventVariant event);

  muse::midi::Event ToMuseMidiEvent(const NoteEvent &dgkEvent) const;
  muse::midi::Event ToMuseMidiEvent(const PedalEvent &pedalEvent) const;
};
} // namespace dgk