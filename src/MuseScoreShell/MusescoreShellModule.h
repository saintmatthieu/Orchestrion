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

#include <memory>

#include "modularity/imodulesetup.h"

namespace dgk::orchestrion
{
class MusescoreShellModule : public muse::modularity::IModuleSetup
{
public:
  std::string moduleName() const override;
  void resolveImports() override;
  void registerResources() override;
  void registerUiTypes() override;
};
} // namespace dgk::orchestrion
