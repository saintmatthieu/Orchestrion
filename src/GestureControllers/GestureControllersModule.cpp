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
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "internal/ComputerKeyboard/ComputerKeyboard.h"
#include "internal/GestureInput.h"

namespace dgk
{
GestureControllersModule::GestureControllersModule()
    : m_gestureInput{std::make_shared<GestureInput>()},
      m_keyboard{std::make_shared<ComputerKeyboard>()}
{
}

std::string GestureControllersModule::moduleName() const
{
  return "Orchestrion";
}

void GestureControllersModule::registerExports()
{
  globalIoc()->registerExport<IGestureInput>(moduleName(), m_gestureInput);
  globalIoc()->registerExport<IComputerKeyboard>(moduleName(), m_keyboard);
}

muse::modularity::IContextSetup *GestureControllersModule::newContext(
    const muse::modularity::ContextPtr &ctx) const
{
  ModuleContextSetup::Hooks hooks;
  hooks.onPreInit = [this](const muse::IApplication::RunMode &mode)
  { onContextPreInit(mode); };
  hooks.onInit = [this](const muse::IApplication::RunMode &mode)
  { onContextInit(mode); };
  return new ModuleContextSetup(ctx, std::move(hooks));
}

void GestureControllersModule::onContextPreInit(
    const muse::IApplication::RunMode &) const
{
  m_keyboard->preInit();
}

void GestureControllersModule::onContextInit(
    const muse::IApplication::RunMode &) const
{
  m_gestureInput->init();
}
} // namespace dgk
