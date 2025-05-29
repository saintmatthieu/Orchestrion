#include "OperatingSystemTouchpadFactory.h"

#ifdef _WIN32
#include "WindowsTouchpad.h"
#else
#include "DummyOperatingSystemTouchpad.h"
#endif

std::unique_ptr<dgk::IOperatingSystemTouchpad>
dgk::createOperatingSystemTouchpad(std::function<void(const TouchpadScan &)> cb)
{
#ifdef _WIN32
  return std::make_unique<WindowsTouchpad>(std::move(cb));
#else
  return std::make_unique<DummyOperatingSystemTouchpad>();
#endif
}