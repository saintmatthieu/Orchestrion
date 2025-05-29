#pragma once

#include "../GestureControllerInternalTypes.h"
#include "IOperatingSystemTouchpad.h"
#include <functional>
#include <memory>

namespace dgk
{
std::unique_ptr<IOperatingSystemTouchpad>
createOperatingSystemTouchpad(std::function<void(const TouchpadScan &)> cb);
}