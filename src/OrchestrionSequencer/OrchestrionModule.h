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

#include "global/modularity/imodulesetup.h"

namespace dgk
{
class Orchestrion;
class GestureControllerConfigurator;

class OrchestrionModule : public muse::modularity::IModuleSetup
{
public:
  OrchestrionModule();

private:
  std::string moduleName() const override;
  void registerExports() override;
  void onInit(const muse::IApplication::RunMode &mode) override;
  void registerUiTypes() override;

  const std::shared_ptr<Orchestrion> m_orchestrion;
  const std::shared_ptr<GestureControllerConfigurator>
      m_midiControllerConfigurator;
};

} // namespace dgk