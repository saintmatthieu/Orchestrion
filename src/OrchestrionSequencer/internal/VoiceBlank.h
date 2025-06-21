#pragma once

#include "IRest.h"

namespace dgk
{
class VoiceBlank : public IRest
{
public:
  VoiceBlank(Tick beginTick, Tick endTick);

private:
  const IChord *AsChord() const override;
  IChord *AsChord() override;
  const IRest *AsRest() const override;
  IRest *AsRest() override;
  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

  const Tick m_beginTick;
  const Tick m_endTick;
};
} // namespace dgk