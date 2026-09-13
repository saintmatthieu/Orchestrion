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
#include "OrnamentGesture.h"

#include <cassert>

namespace dgk
{
OrnamentGesture::OrnamentGesture(std::shared_ptr<IChord> chord,
                                 std::vector<int> pitches, Tick begin, Tick end)
    : m_chord{std::move(chord)}, m_pitches{std::move(pitches)}, m_begin{begin},
      m_end{end}
{
  assert(m_chord);
}

const IChord *OrnamentGesture::AsChord() const { return this; }
IChord *OrnamentGesture::AsChord() { return this; }
const IRest *OrnamentGesture::AsRest() const { return nullptr; }
IRest *OrnamentGesture::AsRest() { return nullptr; }
Tick OrnamentGesture::GetBeginTick() const { return m_begin; }
Tick OrnamentGesture::GetEndTick() const { return m_end; }

std::vector<int> OrnamentGesture::GetPitches() const { return m_pitches; }

float OrnamentGesture::GetVelocity() const { return m_chord->GetVelocity(); }

void OrnamentGesture::SetVelocity(float velocity)
{
  // Recorded on the chord, the last of its notes struck winning.
  m_chord->SetVelocity(velocity);
}

std::optional<float> OrnamentGesture::GetDynamicVelocity() const
{
  return m_chord->GetDynamicVelocity();
}

// Written out, the ornament has nothing left for the sequencer to unfold.
const Ornament *OrnamentGesture::GetOrnament() const { return nullptr; }

double OrnamentGesture::GetNominalBpm() const
{
  return m_chord->GetNominalBpm();
}

const mu::engraving::Chord *OrnamentGesture::GetEngravingChord() const
{
  return m_chord->GetEngravingChord();
}
} // namespace dgk
