#pragma once

namespace dgk
{
class IOperatingSystemTouchpad
{
public:
  virtual ~IOperatingSystemTouchpad() = default; 
  virtual bool isAvailable() const = 0;
};
} // namespace dgk