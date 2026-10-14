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
#include "PerformanceFollower.h"

#include <vector>

namespace dgk
{
/**
 * What a performance of the staves is followed along (see
 * PerformanceFollower): the onsets of their chords, all voices merged, in
 * ticks with repeats, and their ornaments written out as the performer is
 * expected to play them — by the rules the automatic player plays them by,
 * but a trill as one event of its notes, however many of them are played.
 */
std::vector<ScoreEvent> GetScoreEvents(const std::vector<const Staff *> &staves);
} // namespace dgk
