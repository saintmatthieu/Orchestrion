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
  m_items.push_back(std::move(item));
}

bool ModifiableItemRegistry::Unsaved() const
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
  if (!Unsaved())
    return;
  for (IModifiableItem *item : _GetItems())
    item->Save();
  m_unsavedChanged.notify();
}

void ModifiableItemRegistry::RevertToLastSaved()
{
  if (!Unsaved())
    return;
  for (IModifiableItem *item : _GetItems())
    item->RevertChanges();
  m_unsavedChanged.notify();
}

std::unordered_set<IModifiableItem *> ModifiableItemRegistry::_GetItems() const
{
  std::unordered_set<IModifiableItem *> result;
  for (const auto &weakItem : m_items)
    if (const auto item = weakItem.lock())
      result.insert(item.get());
  return result;
}

muse::async::Notification ModifiableItemRegistry::UnsavedChanged() const
{
  return m_unsavedChanged;
}
} // namespace dgk
