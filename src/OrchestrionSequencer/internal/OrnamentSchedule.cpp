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
#include "OrnamentSchedule.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace dgk
{
OrnamentSchedule ScheduleOrnament(const Ornament &ornament, int nominalTicks,
                                  double ticksPerMs)
{
  assert(ticksPerMs > 0.0);
  assert(!ornament.fill.empty());
  const auto timed =
      [ticksPerMs](const OrnamentStep &step, double stretch = 1.0)
  {
    return TimedOrnamentStep{
        step.pitches,
        std::chrono::microseconds{static_cast<long long>(
            std::llround(step.ticks * stretch / ticksPerMs * 1000.0))}};
  };
  const auto total = [](const std::vector<OrnamentStep> &steps)
  {
    int ticks = 0;
    for (const OrnamentStep &step : steps)
      ticks += step.ticks;
    return ticks;
  };

  OrnamentSchedule schedule;
  for (const OrnamentStep &step : ornament.before)
    schedule.steps.push_back(timed(step));

  if (ornament.cycleFill && ornament.after.empty())
  {
    // Nothing closes the ornament: it goes on for as long as the key is held.
    schedule.cycleBegin = schedule.steps.size();
    for (const OrnamentStep &step : ornament.fill)
      schedule.steps.push_back(timed(step));
    schedule.cycleEnd = schedule.steps.size();
    return schedule;
  }

  const int fillTicks = std::max(0, nominalTicks - total(ornament.before) -
                                        total(ornament.after));
  const int cycleTicks = total(ornament.fill);
  const int cycles = ornament.cycleFill && cycleTicks > 0
                         ? std::max(1, fillTicks / cycleTicks)
                         : 1;
  // Spread the cycles evenly over the fill, as MuseScore does.
  const double stretch =
      cycleTicks > 0 ? fillTicks / double(cycles * cycleTicks) : 1.0;
  for (int i = 0; i < cycles; ++i)
    for (const OrnamentStep &step : ornament.fill)
      schedule.steps.push_back(timed(step, stretch));
  for (const OrnamentStep &step : ornament.after)
    schedule.steps.push_back(timed(step));

  // Whatever ends the ornament is held until the release.
  schedule.steps.back().duration = std::chrono::microseconds{0};
  return schedule;
}
} // namespace dgk
