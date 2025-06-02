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
#include "GestureControllersModule.h"
#include "internal/ComputerKeyboard/ComputerKeyboard.h"
#include "internal/GestureControllerConfiguration.h"
#include "internal/GestureControllerSelector.h"
#include <ui/iuiactionsregister.h>

namespace dgk
{
GestureControllersModule::GestureControllersModule()
    : m_gestureControllerConfiguration{std::make_unique<
          GestureControllerConfiguration>()},
      m_keyboard{std::make_shared<ComputerKeyboard>()}
{
}

std::string GestureControllersModule::moduleName() const
{
  return "Orchestrion";
}

void GestureControllersModule::registerExports()
{
  ioc()->registerExport<IGestureControllerSelector>(
      moduleName(), new GestureControllerSelector());
  ioc()->registerExport<IComputerKeyboard>(moduleName(), m_keyboard);
}

void GestureControllersModule::onPreInit(const muse::IApplication::RunMode &)
{
  m_keyboard->preInit();
}

void GestureControllersModule::onAllInited(const muse::IApplication::RunMode &)
{
  m_gestureControllerConfiguration->postInit();
}
} // namespace dgk