/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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
#include "SegmentRegistry.h"

namespace dgk
{
void SegmentRegistry::RegisterSegment(const IMelodySegment *chord,
                                      const mu::engraving::Segment *segment)
{
  m_chords[chord] = segment;
}

void SegmentRegistry::UnregisterSegment(const IMelodySegment *chord)
{
  m_chords.erase(chord);
}

const mu::engraving::Segment *
SegmentRegistry::GetSegment(const IMelodySegment *chord) const
{
  return m_chords.count(chord) ? m_chords.at(chord) : nullptr;
}
} // namespace dgk