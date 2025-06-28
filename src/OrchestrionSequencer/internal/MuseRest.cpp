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
#include "MuseRest.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include <cassert>

namespace dgk
{

namespace me = mu::engraving;

MuseRest::MuseRest(const me::Segment &segment, TrackIndex track,
                   int measurePlaybackTick)
    : MuseMelodySegment(segment, track, measurePlaybackTick)
{
}

const IChord *MuseRest::AsChord() const { return nullptr; }
IChord *MuseRest::AsChord() { return nullptr; }

const IRest *MuseRest::AsRest() const { return this; }
IRest *MuseRest::AsRest() { return this; }

dgk::Tick MuseRest::GetBeginTick() const
{
  return MuseMelodySegment::GetBeginTick();
}

dgk::Tick MuseRest::GetEndTick() const
{
  const me::Segment *segment = &m_segment;
  auto endTick = GetBeginTick();
  while (segment)
  {
    const auto rest =
        dynamic_cast<const me::Rest *>(segment->element(m_track.value));
    if (!rest)
      break;
    endTick += rest->actualTicks().ticks();
    constexpr auto includeOtherVoicesOnStaff = false;
    segment = segment->nextCR(m_track.value, includeOtherVoicesOnStaff);
  }
  return endTick;
}
} // namespace dgk