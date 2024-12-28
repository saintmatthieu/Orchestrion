#include "SynthesizerConnector.h"
#include "VstTrackAudioInput.h"
#include <async/async.h>
#include <audio/internal/audiothread.h>

namespace dgk
{
  switch (meta.type)
  {
  case muse::audio::AudioResourceType::VstPlugin:
    muse::async::Async::call(
        this,
        [this, id = meta.id]
        {
          const auto pluginPtr = std::make_shared<muse::vst::VstPlugin>(id);
          pluginPtr->load();
          pluginPtr->loadingCompleted().onNotify(
              this,
              [this, pluginPtr]
              {
                const auto input =
                    std::make_shared<VstTrackAudioInput>(std::move(pluginPtr));
                audioEngine()->mixer()->addChannel(m_trackId++, input);
              });
        },
        muse::audio::AudioThread::ID);
    break;
  case muse::audio::AudioResourceType::FluidSoundfont:
    break;
  }
}
} // namespace dgk