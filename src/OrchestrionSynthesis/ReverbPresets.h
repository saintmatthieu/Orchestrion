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
#include <translation.h>

#include <array>
#include <optional>
#include <string>

namespace dgk
{
constexpr std::array<ReverbPreset, 4> allReverbPresets{
    ReverbPreset::Room, ReverbPreset::SmallHall, ReverbPreset::LargeHall,
    ReverbPreset::Cathedral};

/** The preset's key in the settings and the effect window. */
inline const char *reverbPresetKey(ReverbPreset preset)
{
  switch (preset)
  {
  case ReverbPreset::Room:
    return "room";
  case ReverbPreset::SmallHall:
    return "smallHall";
  case ReverbPreset::LargeHall:
    return "largeHall";
  case ReverbPreset::Cathedral:
    return "cathedral";
  }
  return "";
}

inline std::optional<ReverbPreset> reverbPresetFromKey(const std::string &key)
{
  for (const auto preset : allReverbPresets)
    if (key == reverbPresetKey(preset))
      return preset;
  return std::nullopt;
}

inline std::string reverbPresetName(ReverbPreset preset)
{
  switch (preset)
  {
  case ReverbPreset::Room:
    return muse::trc("effects", "Room");
  case ReverbPreset::SmallHall:
    return muse::trc("effects", "Small hall");
  case ReverbPreset::LargeHall:
    return muse::trc("effects", "Large hall");
  case ReverbPreset::Cathedral:
    return muse::trc("effects", "Cathedral");
  }
  return {};
}
} // namespace dgk
