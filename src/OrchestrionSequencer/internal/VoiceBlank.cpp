#include "VoiceBlank.h"

namespace dgk
{
VoiceBlank::VoiceBlank(Tick beginTick, Tick endTick)
    : m_beginTick{std::move(beginTick)}, m_endTick{std::move(endTick)}
{
}

bool VoiceBlank::IsChord() const { return false; }

std::vector<int> VoiceBlank::GetPitches() const { return {}; }

Tick VoiceBlank::GetBeginTick() const { return m_beginTick; }

Tick VoiceBlank::GetEndTick() const { return m_endTick; }
} // namespace dgk