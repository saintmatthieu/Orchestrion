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
 * The ornament the engraved chord carries — its grace notes, and the notes
 * its ornament sign or trill line spells out; nullopt for a plain chord.
 */
std::optional<Ornament> BuildOrnament(const mu::engraving::Chord &chord);
} // namespace dgk
