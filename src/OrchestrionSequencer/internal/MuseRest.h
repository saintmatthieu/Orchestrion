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
#pragma once

#include "IRest.h"
#include "MuseMelodySegment.h"

namespace mu::engraving
{
class Note;
class Segment;
} // namespace mu::engraving

namespace dgk
{
class MuseRest : public IRest, private MuseMelodySegment
{
public:
  MuseRest(const mu::engraving::Segment &segment, TrackIndex,
           int measurePlaybackTick);

  const IChord *AsChord() const override;
  IChord *AsChord() override;

  const IRest *AsRest() const override;
  IRest *AsRest() override;

  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;
};
} // namespace dgk