#pragma once

#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"
#include <async/asyncable.h>
#include <audio/isynthresolver.h>
#include <midi/imidioutport.h>
#include <modularity/ioc.h>

namespace dgk
{
class IOrchestrionSequencer;
namespace orchestrion
{
class OrchestrionEventProcessor : public muse::Injectable,
                                  public muse::async::Asyncable
{
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<muse::audio::synth::ISynthResolver> synthResolver;
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort;

public:
  OrchestrionEventProcessor() = default;

  void init();

private:
  void setupCallback(IOrchestrionSequencer& sequencer);
  void onOrchestrionEvent(int track, EventVariant event);
};
} // namespace orchestrion
} // namespace dgk