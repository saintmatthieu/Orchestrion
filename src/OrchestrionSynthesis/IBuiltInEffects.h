/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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

#include "BuiltInEffectTypes.h"
#include <global/io/path.h>
#include <modularity/imoduleinterface.h>

#include <memory>

namespace dgk
{
/**
 * The built-in effects: their meters, and where their parameters live (a
 * JSON file the user edits; changes are picked up while playing).
 */
class IBuiltInEffects : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IBuiltInEffects);

public:
  virtual ~IBuiltInEffects() = default;

  virtual std::shared_ptr<EffectMeter> meter(BuiltInEffect effect) const = 0;
  virtual muse::io::path_t parametersFilePath() const = 0;
};
} // namespace dgk
