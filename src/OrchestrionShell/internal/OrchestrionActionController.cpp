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
#include "OrchestrionActionController.h"
#include <engraving/dom/masterscore.h>
#include <notation/imasternotation.h>

namespace dgk
{
void OrchestrionActionController::init()
{
  dispatcher()->reg(this, "orchestrion-file-open", [this] { onFileOpen(); });
  dispatcher()->reg(this, "orchestrion-file-open-example",
                    [this]
                    {
                      const auto scoreDir =
                          globalConfiguration()->appDataPath() + "scores/";
                      openFromDir(scoreDir);
                    });

  dispatcher()->reg(this, "orchestrion-search-musescore",
                    [this]
                    {
                      const auto url = QUrl("https://musescore.com");
                      interactive()->openUrl(url);
                    });

  dispatcher()->reg(this, "orchestrion-file-help",
                    [this]
                    {
                      const auto url = QUrl("https://github.com/saintmatthieu/Orchestrion/wiki/Find-scores");
                      interactive()->openUrl(url);
                    });
}

void OrchestrionActionController::onFileOpen() const
{
  muse::io::path_t defaultDir = configuration()->lastOpenedProjectsPath();

  if (defaultDir.empty())
    defaultDir = configuration()->userProjectsPath();

  if (defaultDir.empty())
    defaultDir = configuration()->defaultUserProjectsPath();

  openFromDir(defaultDir);
}

void OrchestrionActionController::openFromDir(const muse::io::path_t &dir) const
{
  constexpr auto allExt = "*.mscz *.mxl *.musicxml *.xml";

  std::vector<std::string> filter{
      muse::trc("project", "All supported files") + " (" + allExt + ")",
      muse::trc("project", "MuseScore files") + " (*.mscz)",
      muse::trc("project", "MusicXML files") + " (*.mxl *.musicxml *.xml)"};

  const muse::io::path_t filePath = interactive()->selectOpeningFile(
      muse::qtrc("project", "Open"), dir, filter);

  if (filePath.empty())
    return;

  if (globalContext()->currentProject())
    closeCurrent();

  configuration()->setLastOpenedProjectsPath(muse::io::dirpath(filePath));

  muse::actions::ActionData data;
  QUrl url{filePath.toString()};
  url.setScheme("file");
  data.setArg(0, url);
  dispatcher()->dispatch("file-open", data);
}

void OrchestrionActionController::closeCurrent() const
{
  if (const auto notation = globalContext()->currentMasterNotation())
    // We don't want to get the "Would you like to save?"
    // dialog.
    notation->masterScore()->setSaved(true);
  dispatcher()->dispatch("file-close");
}
} // namespace dgk