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
#include <vector>

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

/**
 * How the chord is rolled, if an arpeggio starts on it or spans it from
 * another voice or staff; nothing for a plain chord, or a bracket (which
 * asks for the notes together).
 */
std::optional<RollDirection> ArpeggioOf(const mu::engraving::Chord &chord);

/**
 * An arpeggio across both hands' staves is played by one hand, the one it
 * starts at — the lowest staff for one rolling up, the highest for one
 * rolling down: the first of its chords there gathers the other staff's,
 * which leave a silence in their hand.
 */
struct CrossStaffArpeggio
{
  /** The other staff's chords this chord gathers, if it gathers any. */
  std::vector<const mu::engraving::Chord *> gathers;
  /** Whether another chord gathers this one. */
  bool gathered = false;
};
CrossStaffArpeggio CrossStaffArpeggioOf(const mu::engraving::Chord &chord);
} // namespace dgk
