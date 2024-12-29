#pragma once

#include "ISynthesizerConnector.h"
#include <async/asyncable.h>
#include <audio/internal/worker/iaudioengine.h>
#include <modularity/ioc.h>

namespace muse::audio
{
class MixerChannel;
}

namespace dgk
{
class SynthesizerConnector : public ISynthesizerConnector,
                             public muse::async::Asyncable,
                             public muse::Injectable
{
  muse::Inject<muse::audio::IAudioEngine> audioEngine;

public:
  SynthesizerConnector() = default;

private:
  void connectVstInstrument(const muse::audio::AudioResourceId &) override;
  void connectFluidSynth() override;
  void disconnect() override;
  std::weak_ptr<muse::audio::MixerChannel> m_currentChannel;
};
} // namespace dgk