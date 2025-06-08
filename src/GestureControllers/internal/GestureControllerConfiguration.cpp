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
#include "GestureControllerConfiguration.h"

#include "global/settings.h"

namespace dgk
{
namespace
{
const std::string module_name("GestureControllers");
const muse::Settings::Key GESTURE_CONTROLLERS(module_name,
                                              "GESTURE_CONTROLLERS");

muse::Val toMuseVal(const GestureControllerTypeSet &types)
{
  muse::ValList list;
  for (const auto &type : types)
    list.push_back(muse::Val(static_cast<int>(type)));
  return muse::Val{list};
}

GestureControllerTypeSet fromMuseVal(const muse::Val &value)
{
  GestureControllerTypeSet types;
  for (const auto &v : value.toList())
    types.insert(static_cast<GestureControllerType>(v.toInt()));
  return types;
}
} // namespace

std::optional<GestureControllerTypeSet>
GestureControllerConfiguration::readSelectedControllers() const
{
  const muse::Val value = muse::settings()->value(GESTURE_CONTROLLERS);
  if (value.isNull() || value.type() != muse::Val::Type::List)
    return std::nullopt;
  return fromMuseVal(value);
}

void GestureControllerConfiguration::writeSelectedControllers(
    const GestureControllerTypeSet &types)
{
  muse::settings()->setLocalValue(GESTURE_CONTROLLERS, toMuseVal(types));
}
} // namespace dgk