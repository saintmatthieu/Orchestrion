#pragma once

#include "GestureControllerTypes.h"

#include "async/notification.h"
#include "modularity/ioc.h"

#include <functional>
#include <optional>
#include <unordered_set>

namespace dgk
{
class IGestureController;
class ITouchpadGestureController;

class IGestureControllerSelector : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IGestureControllerSelector);

public:
  virtual ~IGestureControllerSelector() = default;

  virtual GestureControllerTypeSet functionalControllers() const = 0;

  virtual void setSelectedControllers(GestureControllerTypeSet) = 0;
  virtual GestureControllerTypeSet selectedControllers() const = 0;
  virtual muse::async::Notification selectedControllersChanged() const = 0;

  virtual const IGestureController *
      getSelectedController(GestureControllerType) const = 0;

  virtual muse::async::Notification touchpadControllerChanged() const = 0;
  virtual const ITouchpadGestureController *getTouchpadController() const = 0;
};
} // namespace dgk