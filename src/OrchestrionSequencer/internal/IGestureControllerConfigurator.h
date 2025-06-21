#pragma once

#include "GestureControllers/GestureControllerTypes.h"

#include "modularity/ioc.h"

#include <unordered_set>

namespace dgk
{
class IGestureControllerConfigurator : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IGestureControllerConfigurator);

public:
  virtual ~IGestureControllerConfigurator() = default;
  virtual void
  setSelectedControllers(const GestureControllerTypeSet &) = 0;
};
} // namespace dgk