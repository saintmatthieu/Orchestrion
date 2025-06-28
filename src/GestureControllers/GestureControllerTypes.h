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

#include <unordered_set>
#include <vector>


namespace dgk
{
enum class GestureControllerType
{
  MidiDevice,
  Touchpad,
  Swipe,
  ComputerKeyboard,
  _count
};

using GestureControllerTypeSet = std::unordered_set<GestureControllerType>;

struct Contact
{
  Contact(int id, float x, float y) : uid{id}, x{x}, y{y} {}
  const int uid;
  const float x;
  const float y;
};

using Contacts = std::vector<Contact>;
} // namespace dgk