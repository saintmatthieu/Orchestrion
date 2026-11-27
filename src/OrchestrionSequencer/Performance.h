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

#include <vector>

namespace dgk
{
/** A note of a recorded performance, as transcribed. */
struct PerformedNote
{
  double onsetMs = 0.0;
  double offsetMs = 0.0;
  int pitch = 0;
  int velocity = 0;
};

/** A change of the sustain pedal in a recorded performance. */
struct PedalChange
{
  double ms = 0.0;
  bool down = false;
};

/** A recorded performance, transcribed. */
struct Performance
{
  std::vector<PerformedNote> notes;
  /** In time order. */
  std::vector<PedalChange> pedal;
};
} // namespace dgk
