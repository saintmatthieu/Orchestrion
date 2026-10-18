/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "TiedVoices.h"
#include "IChord.h"
#include "SplitChord.h"
#include "VoiceBlank.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <map>

namespace dgk
{
namespace
{
struct Part
{
  std::vector<int> pitches;
  std::vector<mu::engraving::Note *> notes;
  Tick end{0, 0};
};

struct Lane
{
  std::vector<ChordRestPtr> sequence;
  int end = 0;
};

/** Where the next chord after `i` begins, with repeats; none: never. */
int NextChordTick(const std::vector<ChordRestPtr> &sequence, size_t i)
{
  for (auto j = i + 1; j < sequence.size(); ++j)
    if (sequence[j]->AsChord())
      return sequence[j]->GetBeginTick().withRepeats;
  return std::numeric_limits<int>::max();
}

/**
 * The sequence with a single rest wherever several follow one another, and
 * none before its first chord or after its last.
 */
std::vector<ChordRestPtr>
Tidy(const std::vector<ChordRestPtr> &sequence,
     const std::function<void(const IMelodySegment &)> &onDropped)
{
  std::vector<ChordRestPtr> tidy;
  for (const auto &item : sequence)
    if (item->AsChord())
      tidy.push_back(item);
    else if (tidy.empty())
      onDropped(*item);
    else if (!tidy.back()->AsChord())
    {
      onDropped(*tidy.back());
      onDropped(*item);
      tidy.back() = std::make_shared<VoiceBlank>(tidy.back()->GetBeginTick(),
                                                 item->GetEndTick());
    }
    else
      tidy.push_back(item);
  while (!tidy.empty() && !tidy.back()->AsChord())
  {
    onDropped(*tidy.back());
    tidy.pop_back();
  }
  return tidy;
}
} // namespace

void SplitTiedVoices(
    Staff &staff,
    const std::function<std::vector<TiedNote>(const IChord &)> &tiedNotes,
    const std::function<void(const ChordRestPtr &part,
                             const IMelodySegment &whole)> &onSplit,
    const std::function<void(const IMelodySegment &)> &onDropped)
{
  std::vector<int> voices;
  for (const auto &[voice, sequence] : staff)
    if (voice < numVoices)
      voices.push_back(voice);

  for (const int voice : voices)
  {
    const std::vector<ChordRestPtr> &sequence = staff.at(voice);
    std::vector<ChordRestPtr> kept;
    std::vector<Lane> lanes;
    bool split = false;
    for (size_t i = 0; i < sequence.size(); ++i)
    {
      const ChordRestPtr &item = sequence[i];
      const IChord *chord = item->AsChord();
      const std::vector<TiedNote> notes =
          chord ? tiedNotes(*chord) : std::vector<TiedNote>{};
      if (notes.empty())
      {
        kept.push_back(item);
        continue;
      }

      // The chord's notes, by where they end.
      std::map<int, Part> parts;
      for (const TiedNote &note : notes)
      {
        Part &part = parts[note.end.withRepeats];
        part.pitches.push_back(note.pitch);
        part.notes.push_back(note.note);
        part.end = note.end;
      }
      const int end = chord->GetEndTick().withRepeats;
      if (parts.size() == 1 && parts.begin()->first == end)
      {
        // All end together, with the chord: as engraved.
        kept.push_back(item);
        continue;
      }

      split = true;
      // What stays in the voice: the notes ending last before its next chord.
      const int next = NextChordTick(sequence, i);
      auto stays = parts.upper_bound(next);
      stays = stays == parts.begin() ? parts.end() : std::prev(stays);

      const Tick begin = chord->GetBeginTick();
      const std::shared_ptr<IChord> whole{item, item->AsChord()};
      for (auto it = parts.begin(); it != parts.end(); ++it)
      {
        auto part = std::make_shared<SplitChord>(
            whole, it->second.pitches, it->second.notes, it->second.end);
        onSplit(part, *item);
        if (it == stays)
        {
          kept.push_back(std::move(part));
          if (it->first < end)
            // Over before the engraved chord: the voice rests meanwhile.
            kept.push_back(std::make_shared<VoiceBlank>(it->second.end,
                                                        chord->GetEndTick()));
          continue;
        }
        auto lane =
            std::find_if(lanes.begin(), lanes.end(), [&](const Lane &lane)
                         { return lane.end <= begin.withRepeats; });
        if (lane == lanes.end())
          lane = lanes.emplace(lanes.end());
        else if (lane->end < begin.withRepeats)
        {
          Tick from = begin;
          from += lane->end - begin.withRepeats;
          lane->sequence.push_back(std::make_shared<VoiceBlank>(from, begin));
        }
        lane->sequence.push_back(std::move(part));
        lane->end = it->first;
      }
      if (stays == parts.end())
        // Nothing of it stays: the voice rests meanwhile.
        kept.push_back(
            std::make_shared<VoiceBlank>(begin, chord->GetEndTick()));
    }

    if (!split)
      continue;
    if (auto tidy = Tidy(kept, onDropped); tidy.empty())
      staff.erase(voice);
    else
      staff[voice] = std::move(tidy);
    for (size_t k = 0; k < lanes.size(); ++k)
      staff[voice + numVoices * static_cast<int>(k + 1)] =
          std::move(lanes[k].sequence);
  }
}
} // namespace dgk
