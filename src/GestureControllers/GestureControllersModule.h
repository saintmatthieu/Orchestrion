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

#include "modularity/imodulesetup.h"
#include <QAbstractNativeEventFilter>

namespace dgk
{
class GestureControllerConfiguration;
class GestureControllerSelector;
class ComputerKeyboard;

class GestureControllersModule : public muse::modularity::IModuleSetup
{
public:
  GestureControllersModule();

private:
  std::string moduleName() const override;
  void registerExports() override;
  void onPreInit(const muse::IApplication::RunMode &mode) override;
  void onInit(const muse::IApplication::RunMode &mode) override;
  void onAllInited(const muse::IApplication::RunMode &mode) override;

  const std::unique_ptr<GestureControllerConfiguration>
      m_gestureControllerConfiguration;
  const std::shared_ptr<GestureControllerSelector> m_swipeMidiController;
  const std::shared_ptr<ComputerKeyboard> m_keyboard;
};

} // namespace dgk