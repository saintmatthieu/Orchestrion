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
  sequencer.ChordTransitionTriggered().onReceive(
      this, [this](TrackIndex track, ChordTransition transition)
      { OnChordTransition(track, transition); });
}

void ScoreAnimator::OnChordTransition(TrackIndex track,
                                      const ChordTransition &transition)
{
  const auto interaction = GetInteraction();
  IF_ASSERT_FAILED(interaction) { return; }
  if (transition.activated.chord)
  {
    const auto segment =
        chordRegistry()->GetSegment(transition.activated.chord);
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