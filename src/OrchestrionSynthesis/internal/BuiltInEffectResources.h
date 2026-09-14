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
#include <audio/common/audiotypes.h>
#include <translation.h>

#include <array>
#include <optional>
#include <string>

namespace dgk
{
/**
 * How the built-in effects are known to the audio engine (native effects,
 * MuseScore's own fx type, with these resource ids), to the parameters file
 * and to the effect window (a key), and to the user (a name).
 */
inline muse::audio::AudioResourceId builtInEffectId(BuiltInEffect effect)
{
  switch (effect)
  {
  case BuiltInEffect::Compressor:
    return "Orchestrion Compressor";
  case BuiltInEffect::Limiter:
    return "Orchestrion Limiter";
  }
  return {};
}

constexpr std::array<BuiltInEffect, 2> allBuiltInEffects{
    BuiltInEffect::Compressor, BuiltInEffect::Limiter};

inline std::optional<BuiltInEffect>
builtInEffectOf(const muse::audio::AudioResourceId &id)
{
  for (const auto effect : allBuiltInEffects)
    if (id == builtInEffectId(effect))
      return effect;
  return std::nullopt;
}

inline muse::audio::AudioResourceMeta builtInEffectMeta(BuiltInEffect effect)
{
  muse::audio::AudioResourceMeta meta;
  meta.id = builtInEffectId(effect);
  meta.vendor = "Orchestrion";
  meta.type = muse::audio::resourceTypeName(
      muse::audio::AudioResourceType::NativeEffect);
  return meta;
}

inline std::string builtInEffectName(BuiltInEffect effect)
{
  switch (effect)
  {
  case BuiltInEffect::Compressor:
    return muse::trc("effects", "Compressor");
  case BuiltInEffect::Limiter:
    return muse::trc("effects", "Limiter");
  }
  return {};
}

inline const char *builtInEffectKey(BuiltInEffect effect)
{
  switch (effect)
  {
  case BuiltInEffect::Compressor:
    return "compressor";
  case BuiltInEffect::Limiter:
    return "limiter";
  }
  return "";
}

inline std::optional<BuiltInEffect> builtInEffectFromKey(const std::string &key)
{
  for (const auto effect : allBuiltInEffects)
    if (key == builtInEffectKey(effect))
      return effect;
  return std::nullopt;
}
} // namespace dgk
