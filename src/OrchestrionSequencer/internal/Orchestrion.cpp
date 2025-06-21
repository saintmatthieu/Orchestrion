#include "Orchestrion.h"
#include "OrchestrionSequencerFactory.h"
#include <async/async.h>
#include <audio/internal/audiothread.h>
#include <engraving/dom/masterscore.h>

namespace dgk
{
void Orchestrion::init()
{
  playbackController()->isPlayAllowedChanged().onNotify(
      this,
      [&]()
      {
        const auto masterNotation = globalContext()->currentMasterNotation();
        if (!masterNotation)
        {
          setSequencer(nullptr);
          return;
        }
        auto sequencer =
            OrchestrionSequencerFactory{}.CreateSequencer(*masterNotation);

        // This goes beyond just the official API of the audio module. Is there
        // a better way?
        muse::async::Async::call(
            this, [this]
            { audioEngine()->setMode(muse::audio::RenderMode::RealTimeMode); },
            muse::audio::AudioThread::ID);

        setSequencer(std::move(sequencer));
      });
}

void Orchestrion::setSequencer(IOrchestrionSequencerPtr sequencer)
{
  if (sequencer == m_sequencer)
    return;
  m_sequencer = std::move(sequencer);
  m_sequencerChanged.notify();
}

IOrchestrionSequencerPtr Orchestrion::sequencer() { return m_sequencer; }

muse::async::Notification Orchestrion::sequencerChanged() const
{
  return m_sequencerChanged;
}
} // namespace dgk