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
#include "ConfigurationUtils.h"
#include <QPainter>
#include <QPixmap>

std::filesystem::path dgk::ConfigurationUtils::GetPathToProcessedWallpaper(
    const std::filesystem::path &directory, const std::string &original,
    float opacity)
{
  // This is an image file. We want to add an effect to the wallpaper, save it
  // to a temporary file and set this as wallpaper path.
  // 1. Load the image file
  const QPixmap originalWallpaper =
      QPixmap((directory / original).string().c_str());
  // 2. add a layer of half-transparent white:
  QPixmap wallpaper = originalWallpaper;
  QPainter painter(&wallpaper);
  painter.fillRect(wallpaper.rect(), QColor(255, 255, 255, 255 * opacity));
  painter.end();
  // 3. Save the modified wallpaper to a temporary file
  const std::filesystem::path path = directory / "processed_wallpaper.jpg";
  wallpaper.save(path.string().c_str());
  return path;
}