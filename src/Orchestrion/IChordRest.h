#pragma once

#include "OrchestrionTypes.h"
#include <vector>

namespace dgk
{
class IChordRest
{
public:
  virtual ~IChordRest() = default;
  virtual bool IsChord() const = 0;
  virtual std::vector<int> GetPitches() const = 0;
  virtual Tick GetBeginTick() const = 0;
  virtual Tick GetEndTick() const = 0;
};
} // namespace dgk