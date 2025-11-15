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
#include "ModifiableItemRegistry.h"
#include "IModifiableItem.h"

#include <cassert>

namespace dgk
{
void ModifiableItemRegistry::RegisterItem(std::weak_ptr<IModifiableItem> item)
{
  const auto lockedItem = item.lock();
  if (!lockedItem)
    return;
  lockedItem->ModifiedChanged().onNotify(
      this, [this] { m_modifiedChanged.notify(); },
      muse::async::Asyncable::AsyncMode::AsyncSetOnce);
  m_items.push_back(std::move(item));
}

bool ModifiableItemRegistry::Modified() const
{
  return std::any_of(m_items.begin(), m_items.end(),
                     [](const std::weak_ptr<IModifiableItem> &weakItem)
                     {
                       if (const auto item = weakItem.lock())
                         return item->Modified();
                       return false;
                     });
}

void ModifiableItemRegistry::Save()
{
  if (!Modified())
    return;
  for (IModifiableItem *item : _GetItems())
    item->Save();
  m_modifiedChanged.notify();
}

void ModifiableItemRegistry::RevertToLastSaved()
{
  if (!Modified())
    return;
  for (IModifiableItem *item : _GetItems())
    item->RevertChanges();
  m_modifiedChanged.notify();
}

std::unordered_set<IModifiableItem *> ModifiableItemRegistry::_GetItems() const
{
  std::unordered_set<IModifiableItem *> result;
  for (const auto &weakItem : m_items)
    if (const auto item = weakItem.lock())
      result.insert(item.get());
  return result;
}

muse::async::Notification ModifiableItemRegistry::ModifiedChanged() const
{
  return m_modifiedChanged;
}
} // namespace dgk
