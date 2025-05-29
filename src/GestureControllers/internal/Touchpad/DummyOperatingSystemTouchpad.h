#pragma once

#include "IOperatingSystemTouchpad.h"

namespace dgk
{
class DummyOperatingSystemTouchpad : public IOperatingSystemTouchpad
{
public:
  DummyOperatingSystemTouchpad() = default;
  ~DummyOperatingSystemTouchpad() override = default;

  bool isAvailable() const override { return false; }
};
} // namespace dgk