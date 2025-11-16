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

#include <QApplication>
#include <QWindow>

namespace dgk
{
void OrchestrionActionController::preInit() { qApp->installEventFilter(this); }

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

  projectConfiguration()->setShouldAskSaveLocationType(false);
  projectConfiguration()->setLastUsedSaveLocationType(
      mu::project::SaveLocationType::Local);
}

bool OrchestrionActionController::eventFilter(QObject *watched, QEvent *event)
{
  if ((event->type() == QEvent::Close && watched == mainWindow()->qWindow()) ||
      event->type() == QEvent::Quit)
  {
    constexpr auto closeApp = true;
    const IModifiableItemRegistryPtr registry =
        orchestrion()->modifiableItemRegistry();
    if (registry && registry->Modified())
    {
      const auto notation = globalContext()->currentMasterNotation();
      assert(notation);
      if (notation)
        notation->masterScore()->setSaved(false);
      if (!projectFilesController()->closeOpenedProject(closeApp))
      {
        // Cancel the close event
        event->setAccepted(true);
        return true;
      }
    }
  }
  return QObject::eventFilter(watched, event);
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
    const QUrl url = !args.empty() ? args.arg<QUrl>(0) : QUrl();
    const std::string displayNameOverride =
        args.count() >= 2 ? args.arg<std::string>(1) : std::string();
    const mu::project::ProjectFile projectFile(
        url, QString::fromStdString(displayNameOverride));
    if (projectFile.isValid())
    {
      openProject(projectFile);
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
    if (!registry->Modified())
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