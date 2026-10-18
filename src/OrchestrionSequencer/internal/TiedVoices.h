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

#include <functional>
#include <vector>

namespace mu::engraving
{
class Note;
} // namespace mu::engraving

namespace dgk
{
/** A note a chord strikes, and where its ties end. */
struct TiedNote
{
  int pitch = 0;
  mu::engraving::Note *note = nullptr;
  /** The end of its ties, whichever voices they lead into. */
  Tick end{0, 0};
};

/**
 * Splits the staff's voices where their notes outlast one another, so that
 * every voice played sounds one chord at a time and all of a chord's notes
 * end together — as they would had the score been written so.
 *
 * A note tied into another voice lasts beyond its chord: in Mozart's K. 265,
 * Var. II, bar 2, a C5+G5 is tied over into two inner voices, the G5 for a
 * quarter, the C5 for a bar and more, while its own voice strikes an A5. Each
 * of a chord's notes that end together then make a chord of their own (see
 * SplitChord): the one ending last before its voice's next chord stays in the
 * voice, the others go to lanes split off the voice (see TrackIndex), the
 * first one free when they are struck.
 *
 * `tiedNotes` gives a chord's notes and where their ties end; none for a chord
 * to leave as it is. `onSplit` is told of each part made of a chord, and
 * `onDropped` of each rest the voice no longer has.
 */
void SplitTiedVoices(
    Staff &,
    const std::function<std::vector<TiedNote>(const IChord &)> &tiedNotes,
    const std::function<void(const ChordRestPtr &part,
                             const IMelodySegment &whole)> &onSplit,
    const std::function<void(const IMelodySegment &)> &onDropped);
} // namespace dgk
