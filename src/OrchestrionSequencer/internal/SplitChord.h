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
 * The notes of an engraved chord that end together, where its notes don't
 * (see SplitTiedVoices): a chord of their own, played in the voice or lane
 * they were put in. Everything but its notes and its end is the chord's.
 */
class SplitChord : public IChord
{
public:
  SplitChord(std::shared_ptr<IChord> chord, std::vector<int> pitches,
             std::vector<mu::engraving::Note *> notes, Tick end);

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
  std::vector<mu::engraving::Note *> GetEngravingNotes() const override;
  std::vector<const mu::engraving::Chord *> GetGatheredChords() const override;
  std::optional<RollDirection> GetRoll() const override;
  double GetSoundingFraction() const override;

  /** The engraved chord this is part of. */
  const IChord &GetWholeChord() const { return *m_chord; }

private:
  const std::shared_ptr<IChord> m_chord;
  const std::vector<int> m_pitches;
  const std::vector<mu::engraving::Note *> m_notes;
  const Tick m_end;
};
} // namespace dgk
