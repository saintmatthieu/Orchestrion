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
  dispatcher()->reg(this, "orchestrion-file-open",
                    [this](const muse::actions::ActionData &args)
                    { onFileOpen(args); });
  dispatcher()->reg(this, "orchestrion-file-open-example",
                    [this]
                    {
                      const auto scoreDir =
                          globalConfiguration()->appDataPath() + "scores/";
                      openFromDir(scoreDir);
                    });

  dispatcher()->reg(this, "orchestrion-file-save", [this] { onFileSave(); });
  dispatcher()->reg(this, "orchestrion-file-save-as",
                    [this] { onFileSaveAs(); });

  dispatcher()->reg(this, "orchestrion-search-musescore",
                    [this]
                    {
                      const auto url =
                          QUrl("https://musescore.com/sheetmusic/non-official");
                      interactive()->openUrl(url);
                    });

  dispatcher()->reg(
      this, "orchestrion-file-help",
      [this]
      {
        const auto url = QUrl(
            "https://github.com/saintmatthieu/Orchestrion/wiki/Find-scores");
        interactive()->openUrl(url);
      });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-recording",
                    [this] { toggleRecording(); });
}

muse::io::path_t OrchestrionActionController::fallbackPath() const
{
  auto dir = projectConfiguration()->userProjectsPath();
  if (dir.empty())
    dir = projectConfiguration()->defaultUserProjectsPath();
  return dir;
}

void OrchestrionActionController::onFileOpen(
    const muse::actions::ActionData &args) const
{
  {
    QUrl url = !args.empty() ? args.arg<QUrl>(0) : QUrl();
    QString displayNameOverride =
        args.count() >= 2 ? args.arg<QString>(1) : QString();
    const mu::project::ProjectFile projectFile(url, displayNameOverride);
    if (projectFile.isValid())
    {
      openProject(mu::project::ProjectFile(url, displayNameOverride));
      return;
    }
  }

  muse::io::path_t defaultDir =
      muse::io::dirpath(projectConfiguration()->lastOpenedProjectsPath());
  if (defaultDir.empty())
    defaultDir = fallbackPath();
  openFromDir(defaultDir);
}

void OrchestrionActionController::onFileSave() const
{
  if (const auto registry = orchestrion()->modifiableItemRegistry())
    registry->Save();

  if (const mu::project::INotationProjectPtr notationProject =
          globalContext()->currentProject())
    notationProject->save();
}

namespace
{
constexpr auto allExt = "*.mscz *.mxl *.musicxml *.xml";
static const std::vector<std::string> filter{
    muse::trc("project", "All supported files") + " (" + allExt + ")",
    muse::trc("project", "MuseScore files") + " (*.mscz)",
    muse::trc("project", "MusicXML files") + " (*.mxl *.musicxml *.xml)"};
} // namespace

void OrchestrionActionController::onFileSaveAs() const
{
  if (const auto registry = orchestrion()->modifiableItemRegistry())
    registry->Save();

  muse::io::path_t defaultDir =
      muse::io::dirpath(projectConfiguration()->lastSavedProjectsPath());
  if (defaultDir.empty())
    defaultDir = fallbackPath();
  const muse::io::path_t filePath = interactive()->selectSavingFile(
      muse::qtrc("project", "Save as"), defaultDir, filter);
  projectFilesController()->saveProjectLocally(filePath,
                                               mu::project::SaveMode::SaveAs);
}

void OrchestrionActionController::openFromDir(const muse::io::path_t &dir) const
{
  const muse::io::path_t filePath = interactive()->selectOpeningFile(
      muse::qtrc("project", "Open"), dir, filter);

  if (filePath.empty())
    return;

  const mu::project::ProjectFile projectFile{filePath};
  openProject(projectFile);
}

void OrchestrionActionController::openProject(
    const mu::project::ProjectFile &projectFile) const
{
  const IModifiableItemRegistryPtr registry =
      orchestrion()->modifiableItemRegistry();
  if (const auto notation = globalContext()->currentMasterNotation())
    if (!registry->Unsaved())
    {
      registry->RevertToLastSaved();
      // We don't want to get the "Would you like to save?"
      // dialog.
      notation->masterScore()->setSaved(true);
    }

  constexpr auto closeApp = false;
  projectFilesController()->closeOpenedProject(closeApp);

  projectConfiguration()->setLastOpenedProjectsPath(projectFile.path());

  projectFilesController()->openProject(projectFile);
}

void OrchestrionActionController::toggleRecording() const
{
  sequencerConfig()->setVelocityRecordingEnabled(
      !sequencerConfig()->velocityRecordingEnabled());
}
} // namespace dgk