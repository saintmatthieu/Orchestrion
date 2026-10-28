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
#include "IChord.h"

#include <algorithm>

namespace dgk
{
namespace
{
// A 64th note, at 480 ticks per quarter note: the least a chord let go of
// early sounds for.
constexpr int minSoundingTicks = 30;
} // namespace

Tick IChord::GetReleaseTick() const
{
  const Tick begin = GetBeginTick();
  const Tick end = GetEndTick();
  const double fraction = GetSoundingFraction();
  if (fraction >= 1.0)
    return end;
  const int written = end.withRepeats - begin.withRepeats;
  const int sounding =
      std::min(written, std::max(minSoundingTicks,
                                 static_cast<int>(fraction * written)));
  Tick release = begin;
  release += sounding;
  return release;
}
} // namespace dgk
