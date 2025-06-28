/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
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