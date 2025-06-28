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
#include "Touchpad.h"
#include "OperatingSystemTouchpadFactory.h"
#include "SteadyClock.h"

#include <log.h>

namespace dgk
{
Touchpad::Touchpad()
    : m_osTouchpad{createOperatingSystemTouchpad(
          [this](const TouchpadScan &scan) { m_processor.process(scan); })},
      m_processor{std::make_unique<SteadyClock>()}
{
}

bool Touchpad::isAvailable() const { return m_osTouchpad->isAvailable(); }

muse::async::Channel<Contacts> Touchpad::contactChanged() const
{
  return m_processor.contactChanged();
}
} // namespace dgk