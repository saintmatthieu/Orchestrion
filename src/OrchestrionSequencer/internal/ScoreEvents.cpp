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

#include <map>
#include <set>

namespace dgk
{
namespace
{
struct Onset
{
  std::set<int> pitches;
  bool trill = false;
};

/** Adds the onsets of `chord`, written out, to `onsets`. */
void AddOnsets(const IChord &chord, std::map<int, Onset> &onsets)
{
  const std::vector<WrittenNote> notes = WriteOutOrnament(chord);
  auto tick = chord.GetBeginTick().withRepeats;
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
      o.trill |= notes[i].trill;
    }
    tick += notes[i].ticks;
  }
}
} // namespace

std::vector<ScoreEvent> GetScoreEvents(const std::vector<const Staff *> &staves)
{
  std::map<int, Onset> onsets;
  for (const Staff *staff : staves)
    for (const auto &[voice, sequence] : *staff)
    {
      // Ornaments already written out for the manual mode are written out
      // again from their chord, once, for the trills to be told.
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
        AddOnsets(*chord, onsets);
      }
    }

  std::vector<ScoreEvent> events;
  events.reserve(onsets.size());
  for (const auto &[tick, onset] : onsets)
    events.push_back(
        {tick, {onset.pitches.begin(), onset.pitches.end()}, onset.trill});
  return events;
}
} // namespace dgk
