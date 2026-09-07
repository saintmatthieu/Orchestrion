/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "OrchestrionSequencerFactory.h"
#include "ModifiableItemRegistry.h"
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
#include "engraving/dom/textbase.h"
#include "notation/imasternotation.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <engraving/dom/mscore.h>
#include <string>

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

/**
 * Visits the measures in playback order (repeats unrolled); `measureTick` is
 * the measure's start tick with repeats.
 */
void ForAllMeasures(
    mu::engraving::Score &score,
    std::function<void(const mu::engraving::Measure &, int measureTick)> cb)
{
  auto measureTick = 0;
  for (const auto *repeatSegment : score.repeatList(true))
    for (const auto *measure : repeatSegment->measureList())
    {
      cb(*measure, measureTick);
      measureTick += measure->ticks().ticks();
    }
}

void ForAllSegments(
    mu::engraving::Score &score,
    std::function<void(const mu::engraving::Segment &, int measureTick)> cb)
{
  ForAllMeasures(score,
                 [&](const mu::engraving::Measure &measure, int measureTick)
                 {
                   for (const auto &segment : measure.segments())
                     cb(segment, measureTick);
                 });
}

auto GetChordSequence(mu::engraving::Score &score,
                      ISegmentRegistry &segmentRegistry,
                      IModifiableItemRegistry &modifiableItemRegistry,
                      TrackIndex track)
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
          {
            auto chord =
                std::make_shared<MuseChord>(segment, track, measureTick);
            modifiableItemRegistry.RegisterItem(chord);
            melodySeg = chord;
          }
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
              // let the next iteration create a longer voice blank ...
              return;
            else if (!sequence.empty() && sequence.back()->AsRest())
            {
              // ... neither should we have a rest leading to a blank.
              const auto lastRest = sequence.back().get();
              endTick = lastRest->GetBeginTick();
              segmentRegistry.UnregisterSegment(lastRest);
              sequence.pop_back();
            }
            sequence.push_back(
                std::make_shared<VoiceBlank>(endTick, chordEndTick));
          }
          endTick = chordEndTick;
          segmentRegistry.RegisterSegment(melodySeg, &segment);
          sequence.push_back(std::move(melodySeg));
        }
      });

  if (!sequence.empty() && sequence.back()->AsRest())
    sequence.pop_back();

  return sequence;
}

/**
 * A stretch of depressed pedal, in ticks with repeats.
 */
struct PedalSpan
{
  int onTick = 0;
  int offTick = 0;
};

bool IsOnTracks(const mu::engraving::EngravingItem &item, int beginTrack,
                int endTrack)
{
  const auto track = static_cast<int>(item.track());
  return beginTrack <= track && track < endTrack;
}

/**
 * The pedal lines that begin in `measure`.
 */
std::vector<PedalSpan>
GetPedalLines(const std::multimap<int, mu::engraving::Spanner *> &spanners,
              const mu::engraving::Measure &measure, int measureTick,
              int beginTrack, int endTrack)
{
  using namespace mu::engraving;
  std::vector<PedalSpan> spans;
  const auto end = spanners.lower_bound(measure.endTick().ticks());
  for (auto it = spanners.lower_bound(measure.tick().ticks()); it != end; ++it)
  {
    if (it->second->type() != ElementType::PEDAL ||
        !IsOnTracks(*it->second, beginTrack, endTrack))
      continue;
    const Pedal *pedal = toPedal(it->second);
    const auto onTick = measureTick + (pedal->tick() - measure.tick()).ticks();
    spans.push_back({onTick, onTick + pedal->ticks().ticks()});
  }
  return spans;
}

enum class PedalText
{
  simile, // "Ped. simile": keep pedalling as in the last pedalled measure
  senza,  // "senza Ped.": stop
};

/**
 * The textual pedal instructions in `measure`, with their tick relative to the
 * measure start, in tick order.
 */
std::vector<std::pair<int, PedalText>>
GetPedalTexts(const mu::engraving::Measure &measure, int beginTrack,
              int endTrack)
{
  using namespace mu::engraving;
  std::vector<std::pair<int, PedalText>> texts;
  for (const auto &segment : measure.segments())
    for (const auto *annotation : segment.annotations())
    {
      if (!annotation->isTextBase() ||
          !IsOnTracks(*annotation, beginTrack, endTrack))
        continue;
      auto text =
          static_cast<const TextBase *>(annotation)->plainText().toStdString();
      std::transform(text.begin(), text.end(), text.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      if (text.find("ped") == std::string::npos)
        continue;
      if (text.find("simile") != std::string::npos)
        texts.emplace_back(segment.rtick().ticks(), PedalText::simile);
      else if (text.find("senza") != std::string::npos)
        texts.emplace_back(segment.rtick().ticks(), PedalText::senza);
    }
  return texts;
}

/**
 * The pedalling of the given staves in playback order: the score's pedal lines,
 * plus what "Ped. simile" implies — the pedalling of the last measure that had
 * pedal lines, repeated measure after measure until pedal lines resume or a
 * "senza Ped." is met.
 */
std::vector<PedalSpan> GetPedalSpans(mu::engraving::Score &score,
                                     int beginTrack, int endTrack)
{
  using namespace mu::engraving;
  const auto &spanners = score.spanner();
  std::vector<PedalSpan> spans;
  // The pedal lines of the last measure that had some, relative to its start.
  std::vector<PedalSpan> pattern;
  bool simile = false;
  ForAllMeasures(
      score,
      [&](const Measure &measure, int measureTick)
      {
        const auto lines =
            GetPedalLines(spanners, measure, measureTick, beginTrack, endTrack);
        if (!lines.empty())
        {
          simile = false;
          pattern.clear();
          for (const auto &line : lines)
            pattern.push_back(
                {line.onTick - measureTick, line.offTick - measureTick});
          spans.insert(spans.end(), lines.begin(), lines.end());
        }

        // The part of this measure, if any, to be pedalled "simile".
        std::optional<int> from = simile ? std::make_optional(0) : std::nullopt;
        std::optional<int> until;
        for (const auto &[rtick, text] :
             GetPedalTexts(measure, beginTrack, endTrack))
          if (text == PedalText::simile)
          {
            simile = true;
            if (!from)
              from = rtick;
          }
          else
          {
            simile = false;
            if (from && !until)
              until = rtick;
          }
        if (!from)
          return;
        const auto measureLength = measure.ticks().ticks();
        for (const auto &relative : pattern)
          if (relative.onTick >= *from && relative.onTick < measureLength &&
              relative.onTick < until.value_or(measureLength))
            spans.push_back({measureTick + relative.onTick,
                             measureTick + relative.offTick});
      });
  std::stable_sort(spans.begin(), spans.end(),
                   [](const PedalSpan &a, const PedalSpan &b)
                   { return a.onTick < b.onTick; });
  return spans;
}

PedalSequence GetPedalSequence(mu::engraving::Score &score, int beginStaffIdx,
                               int endStaffIdx)
{
  using namespace mu::engraving;
  std::vector<PedalSequenceItem> sequence;
  for (const auto &span :
       GetPedalSpans(score, beginStaffIdx * VOICES, endStaffIdx * VOICES))
  {
    while (!sequence.empty() && sequence.back().tick > span.onTick)
      // Cut a pedal that overlaps with the beginning of the next.
      sequence.pop_back();

    // Reuse the last item if it coincides in time.
    if (!sequence.empty() && sequence.back().tick == span.onTick)
      sequence.back().down = true;
    else
      sequence.emplace_back(PedalSequenceItem{span.onTick, true});

    sequence.emplace_back(PedalSequenceItem{span.offTick, false});
  }
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

NotationProducts OrchestrionSequencerFactory::CreateSequencer(
    mu::notation::IMasterNotation &masterNotation)
{
  auto &score = *masterNotation.masterScore();
  const auto nStaves = static_cast<int>(score.nstaves());
  const auto rightHandStaff =
      nStaves == 1 ? std::make_optional<int>(0)
                   : GetRightHandStaffIndex(score.repeatList(), nStaves);
  if (!rightHandStaff.has_value())
    return {};

  auto modifiableItems = std::make_shared<ModifiableItemRegistry>();
  Staff rightHand;
  Staff leftHand;
  const auto staff = *rightHandStaff;
  for (auto v = 0; v < numVoices; ++v)
  {
    if (auto sequence = GetChordSequence(
            score, *segmentRegistry(), *modifiableItems, TrackIndex{staff, v});
        !sequence.empty())
      rightHand.emplace(v, std::move(sequence));
    if (auto sequence =
            GetChordSequence(score, *segmentRegistry(), *modifiableItems,
                             TrackIndex{staff + 1, v});
        !sequence.empty())
      leftHand.emplace(v, std::move(sequence));
  }
  auto pedalSequence = GetPedalSequence(score, staff, staff + 2);

  // Copy the staffs before MakeHand moves them.
  Staff rightHandCopy = rightHand;
  Staff leftHandCopy = leftHand;

  auto sequencer = std::make_shared<OrchestrionSequencer>(
      mapper()->instrumentForStaff(staff), MakeHand(staff, rightHand),
      MakeHand(staff + 1, leftHand), std::move(pedalSequence));

  return {sequencer, modifiableItems, std::move(rightHandCopy),
          std::move(leftHandCopy)};
}
} // namespace dgk