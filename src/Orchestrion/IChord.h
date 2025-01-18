#pragma once

#include "IMelodySegment.h"

namespace dgk
{
class IChord : public IMelodySegment
{
public:
  virtual ~IChord() = default;
  virtual std::vector<int> GetPitches() const = 0;
};
} // namespace dgk