#include "OrchestrionSequencerFactory.h"
#include "MuseChord.h"
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

bool TakeIt(const mu::engraving::Segment &segment, int track, bool &prevWasRest)
{
  const auto element = segment.element(track);
  if (!dynamic_cast<const mu::engraving::ChordRest *>(element))
    return false;
  const auto chord = dynamic_cast<const mu::engraving::Chord *>(element);
  Finally finally{[&] { prevWasRest = chord == nullptr; }};
  if (!chord)
    return !prevWasRest;
  return HasUntiedNotes(*chord);
}

bool IsVisible(const mu::engraving::Staff &staff)
{
  return staff.visible() &&
         !staff.staffType(mu::engraving::Fraction(0, 1))->isMuted();
}

// At the moment we are not flexible at all: we look for the first part that has
// two staves and assume this is what we want to play.
std::optional<std::pair<muse::audio::TrackId, size_t /*staff*/>>
GetRightHandStaff(const std::vector<mu::engraving::RepeatSegment *> &repeats,
                  const mu::playback::IPlaybackController::InstrumentTrackIdMap
                      &instrumentTrackIdMap,
                  size_t nScoreTracks)
{
  for (auto track = 0u; track < nScoreTracks; ++track)
    for (const auto &repeat : repeats)
      for (const auto &measure : repeat->measureList())
        for (const auto &segment : measure->segments())
          if (const auto chord = dynamic_cast<const mu::engraving::Chord *>(
                  segment.element(track)))
          {
            const auto staves = chord->part()->staves();
            if (staves.size() == 2 && IsVisible(*chord->staff()))
            {
              const auto instrumentIds = chord->part()->instrumentTrackIdSet();
              if (instrumentIds.size() != 1)
                continue;
              const auto &id = *instrumentIds.begin();
              if (!instrumentTrackIdMap.count(id))
                continue;
              const auto trackId = instrumentTrackIdMap.at(id);
              return {{trackId, chord->staff()->idx()}};
            }
          }
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
                      orchestrion::IChordRegistry &chordRegistry,
                      size_t staffIdx, int voice)
{
  std::vector<ChordPtr> sequence;
  auto prevWasRest = true;
  dgk::Tick endTick{0, 0};
  const auto track = static_cast<int>(staffIdx * mu::engraving::VOICES + voice);
  ForAllSegments(
      score,
      [&](const mu::engraving::Segment &segment, int measureTick)
      {
        if (TakeIt(segment, track, prevWasRest))
        {
          auto chord =
              std::make_shared<MuseChord>(segment, track, voice, measureTick);
          chordRegistry.RegisterChord(chord.get(), &segment);
          const auto chordEndTick = chord->GetEndTick();
          if (endTick.withRepeats > 0 // we don't care if the voice doesn't
                                      // begin at the start.
              && endTick.withRepeats < chord->GetBeginTick().withRepeats)
            // There is a blank in this voice.
            sequence.push_back(
                std::make_shared<VoiceBlank>(endTick, chordEndTick));
          endTick = chordEndTick;
          sequence.push_back(std::move(chord));
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
  OrchestrionSequencer::Hand hand;
  for (auto &[voice, sequence] : staff)
  {
    const int track = staffIdx * mu::engraving::VOICES + voice;
    hand.emplace_back(
        std::make_unique<VoiceSequencer>(track, voice, std::move(sequence)));
  }
  return hand;
}
} // namespace

std::unique_ptr<IOrchestrionSequencer>
OrchestrionSequencerFactory::CreateSequencer(
    mu::notation::IMasterNotation &masterNotation,
    const mu::playback::IPlaybackController::InstrumentTrackIdMap
        &instrumentTrackIdMap)
{
  auto &score = *masterNotation.masterScore();
  const auto rightHandStaff =
      score.nstaves() == 1
          ? std::make_optional(
                std::make_pair<muse::audio::TrackId, size_t>(0, 0))
          : GetRightHandStaff(score.repeatList(), instrumentTrackIdMap,
                              score.ntracks());
  if (!rightHandStaff.has_value())
    return nullptr;
  Staff rightHand;
  Staff leftHand;
  const auto &[trackId, staff] = *rightHandStaff;
  for (auto v = 0; v < numVoices; ++v)
  {
    if (auto sequence = GetChordSequence(score, *chordRegistry(), staff, v);
        !sequence.empty())
      rightHand.emplace(v, std::move(sequence));
    if (auto sequence = GetChordSequence(score, *chordRegistry(), staff + 1, v);
        !sequence.empty())
      leftHand.emplace(v, std::move(sequence));
  }
  auto pedalSequence = GetPedalSequence(score, staff, staff + 2);

  return std::make_unique<OrchestrionSequencer>(
      static_cast<int>(trackId), MakeHand(staff, rightHand),
      MakeHand(staff + 1, leftHand), std::move(pedalSequence));
}

} // namespace dgk