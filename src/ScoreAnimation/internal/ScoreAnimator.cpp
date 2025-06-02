#include "ScoreAnimator.h"
#include "Orchestrion/IChord.h"
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
  sequencer.ChordTransitions().onReceive(
      this, [this](std::map<TrackIndex, ChordTransition> transitions)
      { OnChordTransitions(transitions); });
}

void ScoreAnimator::OnChordTransitions(
    const std::map<TrackIndex, ChordTransition> &transitions)
{
  const auto interaction = GetInteraction();
  IF_ASSERT_FAILED(interaction) { return; }
  for (const auto &[track, transition] : transitions)
  {
    const auto chord = GetPresentChord(transition) ? GetPresentChord(transition)
                       : GetFutureChord(transition) ? GetFutureChord(transition)
                                                    : nullptr;
    if (chord)
    {
      const auto segment = melodySegRegistry()->GetSegment(chord);
      IF_ASSERT_FAILED(segment) { return; }
      interaction->showItem(segment->element(track.value));
    }
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