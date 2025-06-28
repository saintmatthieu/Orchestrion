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
#include "VoiceBlank.h"

namespace dgk
{
VoiceBlank::VoiceBlank(Tick beginTick, Tick endTick)
    : m_beginTick{std::move(beginTick)}, m_endTick{std::move(endTick)}
{
}

const IChord *VoiceBlank::AsChord() const { return nullptr; }

IChord *VoiceBlank::AsChord() { return nullptr; }

const IRest *VoiceBlank::AsRest() const { return this; }

IRest *VoiceBlank::AsRest() { return this; }

Tick VoiceBlank::GetBeginTick() const { return m_beginTick; }

Tick VoiceBlank::GetEndTick() const { return m_endTick; }
} // namespace dgk