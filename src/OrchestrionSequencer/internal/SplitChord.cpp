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
#include "SplitChord.h"

#include <cassert>

namespace dgk
{
SplitChord::SplitChord(std::shared_ptr<IChord> chord, std::vector<int> pitches,
                       std::vector<mu::engraving::Note *> notes, Tick end)
    : m_chord{std::move(chord)}, m_pitches{std::move(pitches)},
      m_notes{std::move(notes)}, m_end{end}
{
  assert(m_chord);
}

const IChord *SplitChord::AsChord() const { return this; }
IChord *SplitChord::AsChord() { return this; }
const IRest *SplitChord::AsRest() const { return nullptr; }
IRest *SplitChord::AsRest() { return nullptr; }
Tick SplitChord::GetBeginTick() const { return m_chord->GetBeginTick(); }
Tick SplitChord::GetEndTick() const { return m_end; }

std::vector<int> SplitChord::GetPitches() const { return m_pitches; }

float SplitChord::GetVelocity() const { return m_chord->GetVelocity(); }

void SplitChord::SetVelocity(float velocity)
{
  // Recorded on the chord, the last of its parts struck winning.
  m_chord->SetVelocity(velocity);
}

std::optional<float> SplitChord::GetDynamicVelocity() const
{
  return m_chord->GetDynamicVelocity();
}

const Ornament *SplitChord::GetOrnament() const
{
  return m_chord->GetOrnament();
}

double SplitChord::GetNominalBpm() const { return m_chord->GetNominalBpm(); }

const mu::engraving::Chord *SplitChord::GetEngravingChord() const
{
  return m_chord->GetEngravingChord();
}

std::vector<mu::engraving::Note *> SplitChord::GetEngravingNotes() const
{
  return m_notes;
}

std::vector<const mu::engraving::Chord *> SplitChord::GetGatheredChords() const
{
  return m_chord->GetGatheredChords();
}

std::optional<RollDirection> SplitChord::GetRoll() const
{
  return m_chord->GetRoll();
}
} // namespace dgk
