#pragma once

#include "OrchestrionTypes.h"
#include <vector>

namespace dgk
{
class IChord;
class IRest;

class IChordRest
{
public:
  virtual ~IChordRest() = default;

  virtual const IChord *AsChord() const = 0;
  virtual IChord *AsChord() = 0;

  virtual const IRest *AsRest() const = 0;
  virtual IRest *AsRest() = 0;

  virtual Tick GetBeginTick() const = 0;
  virtual Tick GetEndTick() const = 0;
};
} // namespace dgk