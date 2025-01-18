#pragma once

#include "IChordRest.h"

namespace dgk
{
class IChord : public IChordRest
{
public:
  virtual ~IChord() = default;
  virtual std::vector<int> GetPitches() const = 0;
};
} // namespace dgk