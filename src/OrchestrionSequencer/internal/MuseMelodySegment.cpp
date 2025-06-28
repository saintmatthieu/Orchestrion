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
#include "MuseMelodySegment.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include <cassert>

namespace dgk
{

namespace me = mu::engraving;

MuseMelodySegment::MuseMelodySegment(const me::Segment &segment,
                                     TrackIndex track, int measurePlaybackTick)
    : m_tick{measurePlaybackTick + segment.rtick().ticks(),
             segment.tick().ticks()},
      m_track{track}, m_segment{segment}
{
}

dgk::Tick MuseMelodySegment::GetBeginTick() const { return m_tick; }
} // namespace dgk