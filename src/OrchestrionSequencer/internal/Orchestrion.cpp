#include "Orchestrion.h"
#include "OrchestrionSequencerFactory.h"
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
        const auto &map = playbackController()->instrumentTrackIdMap();
        if (map.empty())
        {
          setSequencer(nullptr);
          return;
        }
        auto sequencer =
            OrchestrionSequencerFactory{}.CreateSequencer(*masterNotation, map);
        setSequencer(std::move(sequencer));
      });
}

void Orchestrion::setSequencer(std::unique_ptr<IOrchestrionSequencer> sequencer)
{
  if (sequencer == m_sequencer)
    return;
  m_sequencer = std::move(sequencer);
  m_sequencerChanged.notify();
}

IOrchestrionSequencer *Orchestrion::sequencer() { return m_sequencer.get(); }

muse::async::Notification Orchestrion::sequencerChanged() const
{
  return m_sequencerChanged;
}
} // namespace dgk