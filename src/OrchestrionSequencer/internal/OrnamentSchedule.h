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
 * One step of an ornament, timed for the tempo it is struck at.
 */
struct TimedOrnamentStep
{
  std::vector<int> pitches;
  /** Zero: held until the key is released. */
  std::chrono::microseconds duration{0};
};

/**
 * An ornament's steps as they are to sound from the strike, in order.
 */
struct OrnamentSchedule
{
  std::vector<TimedOrnamentStep> steps;
  /**
   * The steps [cycleBegin, cycleEnd) repeat until the key is released — a
   * trill with no closing notes; equal when none do, the last step then being
   * held until the release instead. When cycling, cycleEnd == steps.size().
   */
  size_t cycleBegin = 0;
  size_t cycleEnd = 0;
};

/**
 * Times the ornament of a chord of `nominalTicks` for a performer playing at
 * `ticksPerMs` (> 0): the fixed steps before and after get their nominal
 * durations, and the fill is cycled as often as fits in between (cycleFill),
 * or stretched to fill it.
 */
OrnamentSchedule ScheduleOrnament(const Ornament &ornament, int nominalTicks,
                                  double ticksPerMs);
} // namespace dgk
