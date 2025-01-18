#pragma once

#include "IChordRest.h"

namespace dgk {
class VoiceBlank : public IChordRest {
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