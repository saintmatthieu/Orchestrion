#include "ScoreAnimator.h"
#include "Orchestrion/IOrchestrionSequencer.h"
#include "Orchestrion/OrchestrionTypes.h"
#include <engraving/dom/chord.h>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/note.h>
#include <engraving/dom/tie.h>
#include <log.h>

namespace dgk::orchestrion
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
      this, [this](int track, ChordActivationChange change)
      { OnChordActivationChange(track, change); });
}

void ScoreAnimator::OnChordActivationChange(int track,
                                            const ChordActivationChange &change)
{
  const auto score = GetScore();

  const auto interaction = GetInteraction();
  IF_ASSERT_FAILED(interaction) { return; }
  bool selectionChanged = false;

  for (const auto chord : change.deactivated)
  {
    const auto segment = chordRegistry()->GetSegment(chord);
    IF_ASSERT_FAILED(segment) { continue; }
    const auto items = GetRelevantItems(track, segment);
    std::for_each(items.begin(), items.end(),
                  [&](mu::engraving::EngravingItem *item)
                  {
                    score->deselect(item);
                    selectionChanged = true;
                  });
  }

  if (change.activated)
  {
    const auto segment = chordRegistry()->GetSegment(change.activated);
    IF_ASSERT_FAILED(segment) { return; }
    const auto items = GetRelevantItems(track, segment);
    score->select(items, mu::engraving::SelectType::ADD);
    selectionChanged = true;
    interaction->showItem(segment->element(track));
  }

  if (selectionChanged)
    interaction->selectionChanged().notify();
}

mu::notation::INotationInteractionPtr ScoreAnimator::GetInteraction() const
{
  const auto masterNotation = globalContext()->currentMasterNotation();
  if (!masterNotation)
    return nullptr;
  return masterNotation->notation()->interaction();
}

std::vector<mu::engraving::EngravingItem *>
ScoreAnimator::GetRelevantItems(int track,
                                const mu::engraving::Segment *segment) const
{
  const auto chord =
      dynamic_cast<const mu::engraving::Chord *>(segment->element(track));
  using NoteVector = std::vector<mu::engraving::Note *>;
  const NoteVector notes = chord ? chord->notes() : NoteVector{};
  std::vector<mu::engraving::EngravingItem *> items;
  std::for_each(notes.begin(), notes.end(),
                [&](mu::engraving::Note *note)
                {
                  while (note)
                  {
                    items.emplace_back(note);
                    auto tie = note->tieFor();
                    if (tie)
                      items.emplace_back(tie);
                    note = tie ? tie->endNote() : nullptr;
                  }
                });

  if (notes.empty())
  {
    // Get all consecutive rests, ignoring elements other than chords such as
    // bars, clefs, etc.
    while (segment)
    {
      auto item = segment->element(track);
      if (dynamic_cast<mu::engraving::Chord *>(item))
        break;
      if (const auto rest = dynamic_cast<mu::engraving::Rest *>(item))
        items.emplace_back(rest);
      segment = segment->next();
    }
  }
  return items;
}

mu::engraving::MasterScore *ScoreAnimator::GetScore() const
{
  const auto masterNotation = globalContext()->currentMasterNotation();
  IF_ASSERT_FAILED(masterNotation) { return nullptr; }
  return masterNotation->masterScore();
}

} // namespace dgk::orchestrion