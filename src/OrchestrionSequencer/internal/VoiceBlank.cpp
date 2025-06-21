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