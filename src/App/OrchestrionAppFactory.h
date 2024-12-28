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

#include "CommandOptions.h"
#include <global/iapplication.h>

namespace dgk
{
class OrchestrionAppFactory
{
public:
  std::shared_ptr<muse::IApplication>
  newApp(const dgk::CommandOptions &options) const;

private:
  std::shared_ptr<muse::IApplication>
  newGuiApp(const dgk::CommandOptions &options) const;
  std::shared_ptr<muse::IApplication>
  newConsoleApp(const dgk::CommandOptions &options) const;

  mutable int m_lastID = 0;
};
} // namespace dgk
