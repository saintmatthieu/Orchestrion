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

#include "IMelodySegment.h"

#include <optional>

namespace mu::engraving
{
class Chord;
} // namespace mu::engraving

namespace dgk
{
class IChord : public IMelodySegment
{
public:
  virtual ~IChord() = default;
  virtual std::vector<int> GetPitches() const = 0;
  virtual float GetVelocity() const = 0;
  virtual void SetVelocity(float) = 0;

  //! The playback velocity (0..1) implied by the score's dynamic markings
  //! (p, mf, f, …) in effect at this chord, or std::nullopt if no dynamic
  //! applies. Pre-recorded note velocities take precedence over this.
  virtual std::optional<float> GetDynamicVelocity() const = 0;

  //! The underlying engraving chord this represents (e.g. to map a hovered
  //! on-screen element back to its chord), or nullptr if it no longer
  //! resolves.
  virtual const mu::engraving::Chord *GetEngravingChord() const = 0;
};
} // namespace dgk