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

#include <modularity/imoduleinterface.h>

namespace dgk::orchestrion
{
class IControllerMenuManager : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IControllerMenuManager);

public:
  virtual ~IControllerMenuManager() = default;

  virtual void trySelectDefaultDevice() = 0;
  virtual void onGainedFocus() = 0;
};
} // namespace dgk::orchestrion