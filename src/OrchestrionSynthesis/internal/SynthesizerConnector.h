#pragma once

#include "ISynthesizerConnector.h"
#include <async/asyncable.h>
#include <audio/internal/worker/iaudioengine.h>
#include <audio/iplayback.h>
#include <audio/isynthresolver.h>
#include <context/iglobalcontext.h>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>

namespace dgk
{
class OrchestrionSynthResolver;

class SynthesizerConnector : public ISynthesizerConnector,
                             public muse::async::Asyncable,
                             public muse::Injectable
{
  muse::Inject<muse::audio::IAudioEngine> audioEngine;
  muse::Inject<muse::audio::synth::ISynthResolver> synthResolver;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<muse::audio::IPlayback> playback;
  muse::Inject<mu::playback::IPlaybackController> playbackController;

public:
  SynthesizerConnector();
  void onInit();
  void onAllInited();

private:
  void connectVstInstrument(const muse::audio::AudioResourceId &) override;
  void connectFluidSynth() override;
  void disconnect() override;
  void setInputParams();
  void setOutputParams();

  const std::shared_ptr<OrchestrionSynthResolver> m_orchestrionSynthResolver;
  std::optional<muse::audio::TrackSequenceId> m_trackSequenceId;
  std::vector<std::pair<muse::audio::TrackSequenceId, muse::audio::TrackId>>
      m_tracks;
};
} // namespace dgk