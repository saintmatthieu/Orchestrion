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

#include <optional>

namespace mu::engraving
{
class Chord;
} // namespace mu::engraving

namespace dgk
{
/**
 * The ornament the engraved chord carries — its grace notes, and its ornament
 * sign or trill line, realized after MuseScore's playback rules — as the steps
 * it unfolds into; nullopt for a plain chord. `nominalTicks` is the chord's
 * duration (ties included) and `nominalBps` the score's tempo there, in
 * quarter notes per second: the sub-note value of a trill or mordent depends
 * on the tempo class, and too short a note is not ornamented at all.
 */
std::optional<Ornament> BuildOrnament(const mu::engraving::Chord &chord,
                                      int nominalTicks, double nominalBps);
} // namespace dgk
