#pragma once

#include <async/channel.h>

namespace dgk
{
class IGestureController
{
public:
  virtual ~IGestureController() = default;

  virtual muse::async::Channel<int, float> noteOn() const = 0;
  virtual muse::async::Channel<int> noteOff() const = 0;
};
} // namespace dgk