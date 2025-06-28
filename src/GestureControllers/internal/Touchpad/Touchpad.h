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

#include "ITouchpad.h"
#include "OperatingSystemTouchpadProcessor.h"
#include "IOperatingSystemTouchpad.h"

namespace dgk
{
class Touchpad : public ITouchpad
{
public:
  Touchpad();

  bool isAvailable() const override;
  muse::async::Channel<Contacts> contactChanged() const override;

private:
  const std::unique_ptr<IOperatingSystemTouchpad> m_osTouchpad;
  OperatingSystemTouchpadProcessor m_processor;
};
} // namespace dgk
