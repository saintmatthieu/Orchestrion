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
#include "ParameterStore.h"
#include "thirdparty/DynamicRangeProcessor/DynamicRangeProcessorTypes.h"

#include <map>
#include <memory>

namespace dgk
{
/**
 * What the built-in effects need at run time, shared between the main thread
 * (parameters in, meters out) and the audio engine (which creates the effects).
 */
struct BuiltInEffectRuntime
{
  std::shared_ptr<ParameterStore<DynamicRangeProcessorSettings>> compressor;
  std::shared_ptr<ParameterStore<DynamicRangeProcessorSettings>> limiter;
  std::map<BuiltInEffect, std::shared_ptr<EffectMeter>> meters;
};
} // namespace dgk
