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

#include "IChord.h"

#include <memory>
#include <vector>

namespace dgk
{
/**
 * One note of an ornament written out for the player to strike (the manual
 * mode): a gesture of its own, standing in for its share of the engraved
 * chord. Everything but its pitches and its span is the chord's.
 */
class OrnamentGesture : public IChord
{
public:
  OrnamentGesture(std::shared_ptr<IChord> chord, std::vector<int> pitches,
                  Tick begin, Tick end);

  const IChord *AsChord() const override;
  IChord *AsChord() override;
  const IRest *AsRest() const override;
  IRest *AsRest() override;
  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

  std::vector<int> GetPitches() const override;
  float GetVelocity() const override;
  void SetVelocity(float) override;
  std::optional<float> GetDynamicVelocity() const override;
  const Ornament *GetOrnament() const override;
  double GetNominalBpm() const override;
  const mu::engraving::Chord *GetEngravingChord() const override;

private:
  const std::shared_ptr<IChord> m_chord;
  const std::vector<int> m_pitches;
  const Tick m_begin;
  const Tick m_end;
};
} // namespace dgk
