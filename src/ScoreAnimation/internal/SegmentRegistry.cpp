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
void SegmentRegistry::RegisterSegment(IMelodySegmentWPtr chord,
                                      const mu::engraving::Segment *segment)
{
  m_chords.emplace_back(std::move(chord), segment);
}

void SegmentRegistry::UnregisterSegment(const IMelodySegment *chord)
{
  m_chords.erase(std::remove_if(m_chords.begin(), m_chords.end(),
                                [&chord](const Entry &entry)
                                { return entry.first.lock().get() == chord; }),
                 m_chords.end());
}

const mu::engraving::Segment *
SegmentRegistry::GetSegment(const IMelodySegment *chord) const
{
  const auto it = std::find_if(m_chords.begin(), m_chords.end(),
                               [&chord](const Entry &entry)
                               { return entry.first.lock().get() == chord; });
  return it != m_chords.end() ? it->second : nullptr;
}

std::vector<IMelodySegment *> SegmentRegistry::GetMelodySegments()
{
  std::vector<IMelodySegment *> segments;
  auto it = m_chords.begin();
  ;
  while (it != m_chords.end())
  {
    const auto chordPtr = it->first.lock();
    if (chordPtr)
    {
      segments.push_back(chordPtr.get());
      ++it;
    }
    else
      it = m_chords.erase(it);
  }
  return segments;
}

} // namespace dgk