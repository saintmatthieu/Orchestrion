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

#include <global/iapplication.h>
#include <modularity/imodulesetup.h>
#include <modularity/ioc.h>
#include <project/iprojectconfiguration.h>

namespace dgk::orchestrion
{
class App
{
public:
  INJECT(mu::IApplication, muapplication);
  INJECT(mu::project::IProjectConfiguration, projectConfiguration);

  void addModule(mu::modularity::IModuleSetup *module);
  int run(int argc, char **argv);

private:
  QList<mu::modularity::IModuleSetup *> m_modules;
};
} // namespace dgk::orchestrion
