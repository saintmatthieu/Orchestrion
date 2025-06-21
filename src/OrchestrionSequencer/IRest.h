#pragma once

#include "IMelodySegment.h"

namespace dgk
{
class IRest : public IMelodySegment
{
public:
  virtual ~IRest() = default;
};
} // namespace dgk