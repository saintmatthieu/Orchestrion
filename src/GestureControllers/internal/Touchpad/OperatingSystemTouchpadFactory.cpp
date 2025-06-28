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
#include "OperatingSystemTouchpadFactory.h"

#ifdef _WIN32
#include "WindowsTouchpad.h"
#else
#include "DummyOperatingSystemTouchpad.h"
#endif

std::unique_ptr<dgk::IOperatingSystemTouchpad>
dgk::createOperatingSystemTouchpad(std::function<void(const TouchpadScan &)> cb)
{
#ifdef _WIN32
  return std::make_unique<WindowsTouchpad>(std::move(cb));
#else
  return std::make_unique<DummyOperatingSystemTouchpad>();
#endif
}