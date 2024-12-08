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
#include "OrchestrionOnboardingModel.h"
#include <QUrl>
#include <filesystem>
#include <log.h>
#include <thread>

namespace dgk::orchestrion
{
OrchestrionOnboardingModel::OrchestrionOnboardingModel(QQuickItem *parent)
    : QQuickItem(parent)
{
}

std::optional<muse::actions::ActionData>
OrchestrionOnboardingModel::getFileOpenArgs(
    const StartupProjectFile &projectFile) const
{
  if (projectFile.url.isEmpty())
  {
    const muse::io::path_t path =
        globalConfiguration()->appDataPath() +
        "/scores/Chopin_-_Nocturne_Op_9_No_2_E_Flat_Major.mscz";
    IF_ASSERT_FAILED(std::filesystem::exists(path.toStdString()))
    {
      LOGE() << "File not found: " << path;
      return std::nullopt;
    }
    QUrl url{QString::fromStdString(path.toStdString())};
    url.setScheme("file");
    return muse::actions::ActionData::make_arg2(
        url, QString{"Chopin - Nocturne Op 9 No 2 E Flat Major"});
  }
  else
    return muse::actions::ActionData::make_arg2(
        projectFile.url, projectFile.displayNameOverride);
}

void OrchestrionOnboardingModel::startOnboarding()
{
  if (const auto args =
          getFileOpenArgs(startupScenario()->startupProjectFile());
      args)
    dispatcher()->dispatch("file-open", *args);
}

void OrchestrionOnboardingModel::onGainedFocus()
{
  multiInstances()->notifyAboutGainedFocus();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  midiControllerManager()->onGainedFocus();
}
} // namespace dgk::orchestrion