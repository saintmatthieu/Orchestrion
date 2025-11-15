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

#include "framework/global/modularity/imoduleinterface.h"
#include "framework/global/async/notification.h"

#include <memory>
#include <unordered_set>

namespace dgk
{
class IModifiableItem;

class IModifiableItemRegistry : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IModifiableItemRegistry);

public:
  virtual ~IModifiableItemRegistry() = default;
  virtual void RegisterItem(std::weak_ptr<IModifiableItem>) = 0;
  virtual bool Modified() const = 0;
  virtual void Save() = 0;
  virtual void RevertToLastSaved() = 0;
  virtual muse::async::Notification ModifiedChanged() const = 0;
};

using IModifiableItemRegistryPtr = std::shared_ptr<IModifiableItemRegistry>;
} // namespace dgk