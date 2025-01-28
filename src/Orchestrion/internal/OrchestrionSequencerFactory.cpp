#include "OrchestrionSequencerFactory.h"
#include "MuseChord.h"
#include "MuseRest.h"
#include "OrchestrionSequencer.h"
#include "VoiceBlank.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/pedal.h"
#include "engraving/dom/repeatlist.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/spanner.h"
#include "engraving/dom/staff.h"
#include "notation/imasternotation.h"
#include <cassert>
#include <engraving/dom/mscore.h>

namespace dgk
{
namespace
{
bool HasUntiedNotes(const mu::engraving::Chord &chord)
{
  return std::any_of(chord.notes().begin(), chord.notes().end(),
                     [](const mu::engraving::Note *note)
                     { return !note->tieBack(); });
}

bool TakeIt(const mu::engraving::Segment &segment, TrackIndex track,
            bool &prevWasRest)
{
  const auto element = segment.element(track.value);
  if (!dynamic_cast<const mu::engraving::ChordRest *>(element))
    return false;
  const auto chord = dynamic_cast<const mu::engraving::Chord *>(element);
  Finally finally{[&] { prevWasRest = chord == nullptr; }};
  if (!chord)
    return !prevWasRest;
  return HasUntiedNotes(*chord);
}

// At the moment we are not flexible at all: we look for the first part that has
// two staves and assume this is what we want to play.
std::optional<int> GetRightHandStaffIndex(
    const std::vector<mu::engraving::RepeatSegment *> &repeats, int nStaves)
{
  for (auto staff = 0; staff < nStaves; ++staff)
    for (const auto &repeat : repeats)
      for (const auto &measure : repeat->measureList())
        for (const auto &segment : measure->segments())
          if (const auto chord = dynamic_cast<const mu::engraving::Chord *>(
                  segment.element(TrackIndex{staff, 0}.value)))
            if (chord->part()->staves().size() == 2)
              return staff;
  return std::nullopt;
}

void ForAllSegments(
    mu::engraving::Score &score,
    std::function<void(const mu::engraving::Segment &, int measureTick)> cb)
{
  auto &repeats = score.repeatList(true);
  auto measureTick = 0;
  std::for_each(repeats.begin(), repeats.end(),
                [&](const mu::engraving::RepeatSegment *repeatSegment)
                {
                  const auto &museMeasures = repeatSegment->measureList();
                  std::for_each(museMeasures.begin(), museMeasures.end(),
                                [&](const mu::engraving::Measure *measure)
                                {
                                  const auto &museSegments =
                                      measure->segments();
                                  for (const auto &museSegment : museSegments)
                                    cb(museSegment, measureTick);
                                  measureTick += measure->ticks().ticks();
                                });
                });
}

auto GetChordSequence(mu::engraving::Score &score,
                      ISegmentRegistry &segmentRegistry, TrackIndex track)
{
  std::vector<ChordRestPtr> sequence;
  auto prevWasRest = true;
  dgk::Tick endTick{0, 0};
  ForAllSegments(
      score,
      [&](const mu::engraving::Segment &segment, int measureTick)
      {
        if (TakeIt(segment, track, prevWasRest))
        {
          const auto isChord = dynamic_cast<const mu::engraving::Chord *>(
                                   segment.element(track.value)) != nullptr;

          std::shared_ptr<IMelodySegment> melodySeg;
          if (isChord)
            melodySeg =
                std::make_shared<MuseChord>(segment, track, measureTick);
          else
            melodySeg = std::make_shared<MuseRest>(segment, track, measureTick);

          const auto chordEndTick = melodySeg->GetEndTick();
          if (endTick.withRepeats > 0 // we don't care if the voice doesn't
                                      // begin at the start.
              && endTick.withRepeats < melodySeg->GetBeginTick().withRepeats)
          {
            // There is a blank in this voice ...
            if (melodySeg->AsRest())
              // ... but we shall not insert a voice blank leading to a rest ;
              // let the next iteration create a longer voice blank.
              return;
            sequence.push_back(
                std::make_shared<VoiceBlank>(endTick, chordEndTick));
          }
          endTick = chordEndTick;
          segmentRegistry.RegisterSegment(melodySeg.get(), &segment);
          sequence.push_back(std::move(melodySeg));
        }
      });
  return sequence;
}

PedalSequence GetPedalSequence(mu::engraving::Score &score, int beginStaffIdx,
                               int endStaffIdx)
{
  using namespace mu::engraving;
  const std::multimap<int, Spanner *> &spanners = score.spanner();
  std::vector<PedalSequenceItem> sequence;
  const auto beginTrack = beginStaffIdx * VOICES;
  const auto endTrack = endStaffIdx * VOICES;
  ForAllSegments(score,
                 [&](const Segment &segment, int measureTick)
                 {
                   if (segment.segmentType() != SegmentType::ChordRest)
                     return;
                   const auto tick = segment.tick().ticks();
                   const auto jt = spanners.upper_bound(tick);
                   for (auto it = spanners.lower_bound(tick); it != jt; ++it)
                   {
                     if (it->second->type() != ElementType::PEDAL)
                       continue;
                     const Pedal *pedal = toPedal(it->second);
                     if (pedal->track() < beginTrack ||
                         pedal->track() >= endTrack)
                       continue;
                     const auto onTick = measureTick + segment.rtick().ticks();
                     const auto offTick = onTick + pedal->ticks().ticks();

                     while (!sequence.empty() && sequence.back().tick > onTick)
                       // To be verified, but it looks like there might be some
                       // pedals whose end overlaps with the beginning of the
                       // next pedal.
                       sequence.pop_back();

                     // Reuse the last item if it coincides in time.
                     if (!sequence.empty() && sequence.back().tick == onTick)
                       sequence.back().down = true;
                     else
                       sequence.emplace_back(PedalSequenceItem{onTick, true});

                     sequence.emplace_back(PedalSequenceItem{offTick, false});
                   }
                 });
  return sequence;
}

auto MakeHand(size_t staffIdx, const Staff &staff)
{
  OrchestrionSequencer::HandVoices hand;
  for (auto &[voice, sequence] : staff)
    hand.emplace_back(std::make_unique<VoiceSequencer>(
        TrackIndex{static_cast<int>(staffIdx), voice}, std::move(sequence)));
  return hand;
}
} // namespace

std::unique_ptr<IOrchestrionSequencer>
OrchestrionSequencerFactory::CreateSequencer(
    mu::notation::IMasterNotation &masterNotation)
{
  auto &score = *masterNotation.masterScore();
  const auto nStaves = static_cast<int>(score.nstaves());
  const auto rightHandStaff =
      nStaves == 1 ? std::make_optional<int>(0)
                   : GetRightHandStaffIndex(score.repeatList(), nStaves);
  if (!rightHandStaff.has_value())
    return nullptr;
  Staff rightHand;
  Staff leftHand;
  const auto staff = *rightHandStaff;
  for (auto v = 0; v < numVoices; ++v)
  {
    if (auto sequence =
            GetChordSequence(score, *segmentRegistry(), TrackIndex{staff, v});
        !sequence.empty())
      rightHand.emplace(v, std::move(sequence));
    if (auto sequence = GetChordSequence(score, *segmentRegistry(),
                                         TrackIndex{staff + 1, v});
        !sequence.empty())
      leftHand.emplace(v, std::move(sequence));
  }
  auto pedalSequence = GetPedalSequence(score, staff, staff + 2);

  return std::make_unique<OrchestrionSequencer>(
      mapper()->instrumentForStaff(staff), MakeHand(staff, rightHand),
      MakeHand(staff + 1, leftHand), std::move(pedalSequence));
}

} // namespace dgk