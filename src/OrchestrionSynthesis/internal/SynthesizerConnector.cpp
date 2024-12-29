#include "SynthesizerConnector.h"
#include "VstTrackAudioInput.h"
#include <async/async.h>
#include <audio/internal/audiothread.h>

namespace dgk
{
namespace
{
constexpr auto trackId = 1000;
}

void SynthesizerConnector::connectVstInstrument(
    const muse::audio::AudioResourceId &id)
{
  disconnect();
  muse::async::Async::call(
      this,
      [this, id]
      {
        const auto pluginPtr = std::make_shared<muse::vst::VstPlugin>(id);
        pluginPtr->load();
        pluginPtr->loadingCompleted().onNotify(
            this,
            [this, pluginPtr]
            {
              const auto input =
                  std::make_shared<VstTrackAudioInput>(std::move(pluginPtr));
              const auto result =
                  audioEngine()->mixer()->addChannel(trackId, input);
              if (result.ret)
                m_currentChannel = result.val;
              else
                LOGE() << "Failed to add channel";
            });
      },
      muse::audio::AudioThread::ID);
}

void SynthesizerConnector::disconnect()
{
  if (const auto channel = m_currentChannel.lock())
    muse::async::Async::call(
        this, [this, trackId = channel->trackId()]
        { audioEngine()->mixer()->removeChannel(trackId); },
        muse::audio::AudioThread::ID);
}

void SynthesizerConnector::connectFluidSynth()
{
  disconnect();
  muse::async::Async::call(
      this,
      [this]
      {
        const auto input = std::make_shared<FluidTrackAudioInput>();
        const auto result = audioEngine()->mixer()->addChannel(trackId,
        input); if (result.ret)
          m_currentChannel = result.val;
        else
          LOGE() << "Failed to add channel";
      },
      muse::audio::AudioThread::ID);
}
} // namespace dgk