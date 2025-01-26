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
#include "OrchestrionConfiguration.h"
#include "ConfigurationUtils.h"

namespace dgk
{
void OrchestrionConfiguration::init()
{
  const auto directory =
      std::filesystem::path(
          globalConfiguration()->appDataPath().toStdString()) /
      "wallpapers";
  constexpr auto opacity = 0.8f;
  const std::filesystem::path path =
      ConfigurationUtils::GetPathToProcessedWallpaper(
          directory, "32465347965_95e360edc6_o.jpg", opacity);

  notationConfiguration()->setBackgroundWallpaperPath(path.string());
  notationConfiguration()->setBackgroundUseColor(false);
  notationConfiguration()->setForegroundColor("transparent");
  notationConfiguration()->setForegroundUseColor(true);
}

} // namespace dgk