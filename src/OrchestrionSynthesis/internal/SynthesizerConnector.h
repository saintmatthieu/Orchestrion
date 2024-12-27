#pragma once

#include "ISynthesizerConnector.h"
#include <async/asyncable.h>
#include <audio/internal/worker/iaudioengine.h>
#include <modularity/ioc.h>

namespace dgk::orchestrion
{
class SynthesizerConnector : public ISynthesizerConnector,
                             public muse::async::Asyncable,
                             public muse::Injectable
{
  muse::Inject<muse::audio::IAudioEngine> audioEngine;

public:
  SynthesizerConnector() = default;

private:
  void connectSynthesizer(const muse::audio::AudioResourceMeta &) override;
  muse::audio::TrackId m_trackId = 1000;
};
} // namespace dgk::orchestrion