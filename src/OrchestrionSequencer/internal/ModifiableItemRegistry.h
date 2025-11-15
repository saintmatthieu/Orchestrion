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

#include "IModifiableItemRegistry.h"

#include "framework/global/async/asyncable.h"

#include <vector>

namespace dgk
{
class ModifiableItemRegistry : public IModifiableItemRegistry,
                               public muse::async::Asyncable
{
public:
  ~ModifiableItemRegistry() override = default;

private:
  void RegisterItem(std::weak_ptr<IModifiableItem>) override;
  bool Modified() const override;
  void Save() override;
  void RevertToLastSaved() override;
  muse::async::Notification ModifiedChanged() const override;

  std::unordered_set<IModifiableItem *> _GetItems() const;

  std::vector<std::weak_ptr<IModifiableItem>> m_items;
  muse::async::Notification m_modifiedChanged;
};
} // namespace dgk