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
#include "log.h"
#include <QUrl>
#include <filesystem>

namespace dgk::orchestrion
{
OrchestrionOnboardingModel::OrchestrionOnboardingModel(QQuickItem *parent)
    : QQuickItem(parent)
{
}

void OrchestrionOnboardingModel::startOnboarding()
{
  const muse::io::path_t path =
      globalConfiguration()->appDataPath() +
      "/scores/Chopin_-_Nocturne_Op_9_No_2_E_Flat_Major.mscz";
  IF_ASSERT_FAILED(std::filesystem::exists(path.toStdString()))
  {
    LOGE() << "File not found: " << path;
    return;
  }
  QUrl url{QString::fromStdString(path.toStdString())};
  url.setScheme("file");
  dispatcher()->dispatch(
      "file-open",
      muse::actions::ActionData::make_arg2(
          std::move(url), QString{"Chopin - Nocturne Op 9 No 2 E Flat Major"}));
}
} // namespace dgk::orchestrion