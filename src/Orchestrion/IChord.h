#pragma once

#include "IChordRest.h"

namespace dgk
{
class IChord : public IChordRest
{
public:
  virtual ~IChord() = default;
};
} // namespace dgk