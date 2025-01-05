#include "ScoreAnimator.h"
#include "Orchestrion/IOrchestrionSequencer.h"
#include "Orchestrion/OrchestrionTypes.h"
#include <engraving/dom/chord.h>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/note.h>
#include <engraving/dom/tie.h>
#include <log.h>

namespace dgk
{
void ScoreAnimator::init()
{
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this]
                                             {
                                               const auto sequencer =
                                                   orchestrion()->sequencer();
                                               if (!sequencer)
                                                 return;
                                               Subscribe(*sequencer);
                                             });
  if (const auto sequencer = orchestrion()->sequencer())
    Subscribe(*sequencer);
}

void ScoreAnimator::Subscribe(const IOrchestrionSequencer &sequencer)
{
  sequencer.ChordActivationChanged().onReceive(
      this, [this](TrackIndex track, ChordActivationChange change)
      { OnChordActivationChange(track, change); });
}

void ScoreAnimator::OnChordActivationChange(TrackIndex track,
                                            const ChordActivationChange &change)
{
  const auto interaction = GetInteraction();
  IF_ASSERT_FAILED(interaction) { return; }
  if (change.activated)
  {
    const auto segment = chordRegistry()->GetSegment(change.activated);
    IF_ASSERT_FAILED(segment) { return; }
    interaction->showItem(segment->element(track.value));
  }
  interaction->selectionChanged().notify();
}

mu::notation::INotationInteractionPtr ScoreAnimator::GetInteraction() const
{
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (!masterNotation)
    return nullptr;
  return masterNotation->notation()->interaction();
}
} // namespace dgk