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
#pragma once

#include "OrnamentSchedule.h"
#include "Performance.h"
#include "PerformanceAligner.h"
#include "RecordingTimeline.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

namespace dgk
{
/**
 * How a performance played the score: each note's velocity, the notes of
 * its ornaments, its pace.
 */
class PerformedScore
{
public:
  /** How loud the note of `pitch` struck at `tick` (with repeats) was, 0 to 1.
   */
  std::optional<float> Velocity(int tick, int pitch) const;
  /**
   * The notes performed of `pitches` at the events from `begin` to before
   * `end` (ticks with repeats), the notes of a trill beginning there
   * included: an ornament's, in time order.
   */
  std::vector<PerformedNote> NotesBetween(int begin, int end,
                                          const std::set<int> &pitches) const;
  /**
   * When to let go of a chord struck at `begin` (ticks with repeats), to
   * end before `end`, of `pitches`: as the last of its notes performed
   * (NotesBetween) was, in ticks with repeats; never before `minTick`, nor
   * after `end`. None if none of them was performed.
   */
  std::optional<int> ReleaseTick(int begin, int end, int minTick,
                                 const std::set<int> &pitches) const;
  /** How fast the score was performed around `tick`, in ticks per ms. */
  std::optional<double> TicksPerMsAt(int tick) const;
  /** When the note of `pitch` struck at `tick` was, in ms. */
  std::optional<double> Onset(int tick, int pitch) const;
  /**
   * When the event at `tick` was struck: its earliest note performed (a
   * hand may come after the other, a chord be rolled).
   */
  std::optional<double> StrikeMs(int tick) const;

private:
  friend struct PerformanceBuilder;
  struct Event
  {
    /** The notes matched to the event's pitches. */
    std::vector<PerformedNote> struck;
    /** Those of a trill beginning there, but its first. */
    std::vector<PerformedNote> trill;
    std::optional<double> ms;
  };
  std::map<int, Event> m_events;
  std::shared_ptr<const RecordingTimeline> m_timeline;
};

/** What the score is played with to play it as a performance was. */
struct PerformancePlayback
{
  /**
   * When each event was struck (PerformedScore::StrikeMs, else when the
   * alignment says), unsmoothed; null if none was. A repeat the performer
   * skipped is a skip.
   */
  std::shared_ptr<const RecordingTimeline> timeline;
  /** Null if no note was performed. */
  std::shared_ptr<const PerformedScore> performed;
};

/** From a performance and its alignment to `events` (see AlignPerformance). */
PerformancePlayback
MakePerformancePlayback(const std::vector<ScoreEvent> &events,
                        const std::vector<PerformedNote> &notes,
                        const PerformanceAlignment &alignment);

/**
 * An ornament as performed (see PerformedScore::NotesBetween), struck at
 * `strikeMs` of the performance (its event's time): its notes in steps of
 * those struck together, each starting when it did after the strike, by
 * `msScale` (the time now for a ms of the performance's), the last held.
 * What was played before the strike is cut short: the step under way then
 * is the first. Each step as loud, relative to `reference` (the strike's
 * velocity, 0 to 1), as its loudest note was. None if fewer than two steps
 * are left.
 */
std::optional<OrnamentSchedule>
PerformedOrnament(const std::vector<PerformedNote> &notes, double strikeMs,
                  double msScale, float reference);
} // namespace dgk
