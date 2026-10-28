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
#include "ScoreEvents.h"
#include "IChord.h"
#include "OrnamentGesture.h"
#include "OrnamentSchedule.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

namespace dgk
{
namespace
{
struct Onset
{
  std::set<int> pitches;
  int writtenTick = 0;
  std::set<int> trillPitches;
  int trillEnd = 0;
  bool rolled = false;
};

/** Adds the onsets of `chord`, written out, to `onsets`. */
void AddOnsets(const IChord &chord, std::map<int, Onset> &onsets)
{
  const std::vector<WrittenNote> notes = WriteOutOrnament(chord);
  const Tick begin = chord.GetBeginTick();
  auto tick = begin.withRepeats;
  auto onset = tick;
  for (size_t i = 0; i < notes.size(); ++i)
  {
    // A trill's notes all belong to the onset of its first.
    if (!(i > 0 && notes[i].trill && notes[i - 1].trill))
      onset = tick;
    if (!notes[i].pitches.empty())
    {
      auto &o = onsets[onset];
      o.pitches.insert(notes[i].pitches.begin(), notes[i].pitches.end());
      o.writtenTick = begin.withoutRepeats + (onset - begin.withRepeats);
      o.rolled |= chord.GetRoll().has_value();
    }
    tick += notes[i].ticks;
    if (notes[i].trill)
    {
      auto &o = onsets[onset];
      o.trillPitches.insert(notes[i].pitches.begin(), notes[i].pitches.end());
      o.trillEnd = std::max(o.trillEnd, tick);
    }
  }
}

/**
 * Calls `f` on each chord of the staves, an ornamented one once: ornaments
 * already written out for the manual mode are written out again from their
 * chord.
 */
void ForEachChord(const std::vector<const Staff *> &staves,
                  const std::function<void(const IChord &)> &f)
{
  for (const Staff *staff : staves)
    for (const auto &[voice, sequence] : *staff)
    {
      const IChord *lastOrnamented = nullptr;
      for (const ChordRestPtr &segment : sequence)
      {
        const IChord *chord = segment->AsChord();
        if (!chord)
          continue;
        if (const auto gesture = dynamic_cast<const OrnamentGesture *>(chord))
        {
          chord = &gesture->GetOrnamentedChord();
          if (chord == lastOrnamented)
            continue;
          lastOrnamented = chord;
        }
        f(*chord);
      }
    }
}
} // namespace

std::vector<ScoreEvent> GetScoreEvents(const std::vector<const Staff *> &staves)
{
  std::map<int, Onset> onsets;
  ForEachChord(staves, [&](const IChord &chord) { AddOnsets(chord, onsets); });

  std::vector<ScoreEvent> events;
  events.reserve(onsets.size());
  for (const auto &[tick, onset] : onsets)
    events.push_back({tick,
                      {onset.pitches.begin(), onset.pitches.end()},
                      onset.writtenTick,
                      {onset.trillPitches.begin(), onset.trillPitches.end()},
                      onset.trillEnd,
                      onset.rolled});
  SpreadTrills(events);
  return events;
}

std::vector<ScoreNote> GetScoreNotes(const std::vector<const Staff *> &staves)
{
  std::vector<ScoreNote> notes;
  ForEachChord(
      staves,
      [&](const IChord &chord)
      {
        auto tick = chord.GetBeginTick().withRepeats;
        const auto roll = chord.GetRoll();
        const std::vector<WrittenNote> written = WriteOutOrnament(chord);
        // A staccato chord is let go of early (its last note, if it
        // is ornamented).
        const int release = chord.GetReleaseTick().withRepeats;
        for (const WrittenNote &note : written)
        {
          const int end =
              &note == &written.back()
                  ? std::max(tick + 1, std::min(tick + note.ticks, release))
                  : tick + note.ticks;
          std::vector<int> pitches = note.pitches;
          if (roll)
            // Rolled, in its order, each note a little after the
            // one before; all released together.
            std::sort(pitches.begin(), pitches.end(),
                      [up = *roll == RollDirection::up](int a, int b)
                      { return up ? a < b : a > b; });
          for (size_t i = 0; i < pitches.size(); ++i)
          {
            const int delay =
                roll ? static_cast<int>(i) * arpeggioNoteTicks : 0;
            notes.push_back({std::min(tick + delay, end - 1), end, pitches[i]});
          }
          tick += note.ticks;
        }
      });
  std::sort(notes.begin(), notes.end(),
            [](const ScoreNote &a, const ScoreNote &b)
            { return a.onTick < b.onTick; });
  return notes;
}

void SpreadTrills(std::vector<ScoreEvent> &events)
{
  const auto add = [](std::vector<int> &into, const std::vector<int> &pitches)
  {
    into.insert(into.end(), pitches.begin(), pitches.end());
    std::sort(into.begin(), into.end());
    into.erase(std::unique(into.begin(), into.end()), into.end());
  };
  for (size_t i = 0; i < events.size(); ++i)
  {
    // Copied: the events within it may be the trill's, and are changed.
    const std::vector<int> trillPitches = events[i].trillPitches;
    const int trillEnd = events[i].trillEnd;
    for (auto j = i + 1; j < events.size() && events[j].tick < trillEnd; ++j)
    {
      add(events[j].trillPitches, trillPitches);
      events[j].trillEnd = std::max(events[j].trillEnd, trillEnd);
    }
  }
}
} // namespace dgk
