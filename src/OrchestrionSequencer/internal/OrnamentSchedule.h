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

#include "OrchestrionTypes.h"

#include <chrono>
#include <vector>

namespace dgk
{
/**
 * One note of a struck chord's unfolding, timed for the tempo it is struck
 * at.
 */
struct TimedOrnamentStep
{
  std::vector<int> pitches;
  /** Zero: held until the gesture that leaves the chord. */
  std::chrono::microseconds duration{0};
};

/**
 * How a struck chord sounds, from the strike to the gesture that leaves it.
 */
struct OrnamentSchedule
{
  /**
   * From the strike, in order. The last step is held until the leaving
   * gesture, unless steps cycle.
   */
  std::vector<TimedOrnamentStep> steps;
  /**
   * The steps [cycleBegin, cycleEnd) repeat until the leaving gesture — a
   * trill; equal when none do. When cycling, cycleEnd == steps.size().
   */
  size_t cycleBegin = 0;
  size_t cycleEnd = 0;
  /**
   * Played at the leaving gesture, before what it moves on to (grace notes
   * after the chord). All timed.
   */
  std::vector<TimedOrnamentStep> closing;
};

/** A chord with nothing to it: struck, held until the leaving gesture. */
OrnamentSchedule PlainChord(std::vector<int> pitches);

/**
 * Times the ornament of a chord of `mainPitches` and `nominalTicks` for a
 * performer playing at `ticksPerMs` (> 0). Ornament notes are 32nds, and
 * grace notes the values the score gives them (see Ornament), at that tempo
 * — playing slowly slows the ornaments too — down to a physical minimum
 * length. A figure the note has
 * room for is played at that speed, the main note then held; one the note
 * hasn't room for is compressed to fill it, running into the next note. A
 * trill is 32nds kept up until the leaving gesture; a note without room for
 * three of them gets the short trill's figure, compressed.
 */
OrnamentSchedule ScheduleOrnament(const Ornament &ornament,
                                  const std::vector<int> &mainPitches,
                                  int nominalTicks, double ticksPerMs);
} // namespace dgk
