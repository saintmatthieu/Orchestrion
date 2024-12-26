#pragma once

#include "IChord.h"

namespace dgk {
class VoiceBlank : public IChord {
public:
  VoiceBlank(Tick beginTick, Tick endTick);

private:
  bool IsChord() const override;
  std::vector<int> GetPitches() const override;
  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

  const Tick m_beginTick;
  const Tick m_endTick;
};
} // namespace dgk