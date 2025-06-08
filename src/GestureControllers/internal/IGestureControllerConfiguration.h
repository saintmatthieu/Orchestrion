#pragma once

#include "GestureControllerTypes.h"

#include "modularity/ioc.h"

#include <optional>

namespace dgk
{
class IGestureControllerConfiguration : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IGestureControllerConfiguration);

public:
  virtual void
  writeSelectedControllers(const GestureControllerTypeSet &types) = 0;
  virtual std::optional<GestureControllerTypeSet> readSelectedControllers() const = 0;
};
} // namespace dgk