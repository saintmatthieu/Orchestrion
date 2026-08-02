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

#include "global/settings.h"

namespace dgk
{
namespace
{
const std::string module_name("Orchestrion");
const muse::Settings::Key FIRST_RUN_WELCOME_ACKNOWLEDGED(
    module_name, "FIRST_RUN_WELCOME_ACKNOWLEDGED");
} // namespace

void OrchestrionConfiguration::init()
{
  muse::settings()->setDefaultValue(FIRST_RUN_WELCOME_ACKNOWLEDGED,
                                    muse::Val{false});

  const auto config = globalConfiguration();
  const auto directory = config->appDataPath().toStdString() + "wallpapers";
  constexpr auto opacity = 0.0f;
  const std::string path = ConfigurationUtils::GetPathToProcessedWallpaper(
      directory, config->userAppDataPath().toStdString(),
      "orchestrion_parchment.jpg", opacity);

  notationConfiguration()->setBackgroundWallpaperPath(path);
  notationConfiguration()->setBackgroundUseColor(false);
  notationConfiguration()->setForegroundColor("transparent");
  notationConfiguration()->setForegroundUseColor(true);
}

bool OrchestrionConfiguration::firstRunWelcomeAcknowledged() const
{
  return muse::settings()->value(FIRST_RUN_WELCOME_ACKNOWLEDGED).toBool();
}

void OrchestrionConfiguration::setFirstRunWelcomeAcknowledged(bool acknowledged)
{
  muse::settings()->setSharedValue(FIRST_RUN_WELCOME_ACKNOWLEDGED,
                                   muse::Val{acknowledged});
}

} // namespace dgk