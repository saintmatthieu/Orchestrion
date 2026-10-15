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
#include "MuseScoreShell/OrchestrionActionIds.h"
#include "TakeRecorder.h"
#include "TakeReplayer.h"
#include <async/async.h>
#include <engraving/dom/masterscore.h>
#include <engraving/editing/transaction/undostack.h>
#include <global/log.h>
#include <global/translation.h>
#include <notation/imasternotation.h>
#include <notation/inotation.h>
#include <notation/inotationundostack.h>
#include <project/inotationproject.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QWindow>

namespace dgk
{
void OrchestrionActionController::preInit() { qApp->installEventFilter(this); }

OrchestrionActionController::OrchestrionActionController()
    : m_takeRecorder{std::make_unique<TakeRecorder>()},
      m_takeReplayer{std::make_unique<TakeReplayer>(*m_takeRecorder)}
{
}

OrchestrionActionController::~OrchestrionActionController() = default;

void OrchestrionActionController::init()
{
  m_takeRecorder->init();
  m_takeReplayer->init();
  dispatcher()->reg(this, actionIds::toggleTakeRecording,
                    [this]
                    {
                      sequencerConfig()->setTakeRecordingEnabled(
                          !sequencerConfig()->takeRecordingEnabled());
                    });
  dispatcher()->reg(this, actionIds::replayTake, [this] { replayTake(); });

  dispatcher()->reg(this, "orchestrion-file-open",
                    [this](const muse::actions::ActionData &args)
                    { onFileOpen(args); });

  dispatcher()->reg(this, "orchestrion-file-save", [this] { onFileSave(); });
  dispatcher()->reg(this, "orchestrion-file-save-as",
                    [this] { onFileSaveAs(); });
  dispatcher()->reg(this, actionIds::clearRecentFiles,
                    [this] { recentFilesController()->clearRecentFiles(); });

  dispatcher()->reg(this, "orchestrion-search-musescore",
                    [this]
                    {
                      const auto url =
                          QUrl("https://musescore.com/sheetmusic/non-official");
                      platformInteractive()->openUrl(url);
                    });

  dispatcher()->reg(
      this, "orchestrion-file-help",
      [this]
      {
        const auto url = QUrl(
            "https://github.com/saintmatthieu/Orchestrion/wiki/Find-scores");
        platformInteractive()->openUrl(url);
      });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-recording",
                    [this] { toggleRecording(); });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-note-info",
                    [this]
                    {
                      sequencerConfig()->setNoteInfoTooltipEnabled(
                          !sequencerConfig()->noteInfoTooltipEnabled());
                    });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-tempo-visualization",
                    [this]
                    {
                      sequencerConfig()->setTempoVisualizationEnabled(
                          !sequencerConfig()->tempoVisualizationEnabled());
                    });
  dispatcher()->reg(this, actionIds::toggleJumpAnticipation,
                    [this]
                    {
                      sequencerConfig()->setJumpAnticipationEnabled(
                          !sequencerConfig()->jumpAnticipationEnabled());
                    });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-grading",
                    [this]
                    {
                      sequencerConfig()->setGradingEnabled(
                          !sequencerConfig()->gradingEnabled());
                    });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-persistent-timing-marks",
                    [this]
                    {
                      sequencerConfig()->setPersistentTimingMarksEnabled(
                          !sequencerConfig()->persistentTimingMarksEnabled());
                    });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-hand-sync-score",
                    [this]
                    {
                      sequencerConfig()->setHandSyncScoreEnabled(
                          !sequencerConfig()->handSyncScoreEnabled());
                    });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-dynamics-score",
                    [this]
                    {
                      sequencerConfig()->setDynamicsScoreEnabled(
                          !sequencerConfig()->dynamicsScoreEnabled());
                    });
  dispatcher()->reg(this, actionIds::toggleGradingExposure,
                    [this]
                    {
                      sequencerConfig()->setGradingExposed(
                          !sequencerConfig()->gradingExposed());
                    });
  dispatcher()->reg(this, "orchestrion-advanced-toggle-proportional-spacing",
                    [this]
                    {
                      sequencerConfig()->setTimeProportionalSpacingEnabled(
                          !sequencerConfig()->timeProportionalSpacingEnabled());
                    });

  dispatcher()->reg(
      this, actionIds::playModePerformance,
      [this] { orchestrion()->setPlayMode(PlayMode::replayPerformance); });
  dispatcher()->reg(
      this, actionIds::playModeFittedTempo,
      [this] { orchestrion()->setPlayMode(PlayMode::replayFittedTempo); });
  dispatcher()->reg(this, actionIds::playModeMetronome, [this]
                    { orchestrion()->setPlayMode(PlayMode::metronome); });

  dispatcher()->reg(
      this, actionIds::ornamentsDisabled,
      [this] { sequencerConfig()->setOrnamentMode(OrnamentMode::disabled); });
  dispatcher()->reg(
      this, actionIds::ornamentsManual,
      [this] { sequencerConfig()->setOrnamentMode(OrnamentMode::manual); });
  dispatcher()->reg(
      this, actionIds::ornamentsAutomatic,
      [this] { sequencerConfig()->setOrnamentMode(OrnamentMode::automatic); });

  // The master effect chain (the Effects menu); the argument is the
  // effect's id.
  dispatcher()->reg(this, actionIds::addEffect,
                    [this](const muse::actions::ActionData &args)
                    { effectChain()->addEffect(args.arg<std::string>(0)); });
  dispatcher()->reg(this, actionIds::removeEffect,
                    [this](const muse::actions::ActionData &args)
                    { effectChain()->removeEffect(args.arg<std::string>(0)); });
  dispatcher()->reg(this, actionIds::editEffect,
                    [this](const muse::actions::ActionData &args)
                    { effectChain()->openEditor(args.arg<std::string>(0)); });
  dispatcher()->reg(this, actionIds::toggleMidiKeyboardIcon,
                    [this]
                    {
                      sequencerConfig()->setMidiKeyboardIconVisible(
                          !sequencerConfig()->midiKeyboardIconVisible());
                    });

  dispatcher()->reg(this, actionIds::togglePedalIndicator,
                    [this]
                    {
                      sequencerConfig()->setPedalIndicatorVisible(
                          !sequencerConfig()->pedalIndicatorVisible());
                    });

  // View ▸ Theme. The framework persists the choice and hot-reloads the
  // palette, so switching is the one call; everything that paints from
  // the palette follows its currentThemeChanged().
  dispatcher()->reg(
      this, actionIds::setGoldTheme, [this]
      { uiConfiguration()->setCurrentTheme(muse::ui::LIGHT_THEME_CODE); });
  dispatcher()->reg(
      this, actionIds::setSilverTheme, [this]
      { uiConfiguration()->setCurrentTheme(muse::ui::DARK_THEME_CODE); });

  dispatcher()->reg(this, "view-toggle-fullscreen",
                    [this]
                    {
                      QWindow *w = mainWindow()->qWindow();
                      if (!w)
                        return;
                      if (w->visibility() == QWindow::FullScreen)
                        w->showMaximized();
                      else
                        w->showFullScreen();
                    });

  dispatcher()->reg(this, "prev",
                    [this]
                    {
                      if (auto seq = orchestrion()->sequencer())
                        seq->GoToPrevNoteonTick();
                    });
  dispatcher()->reg(this, "next",
                    [this]
                    {
                      if (auto seq = orchestrion()->sequencer())
                        seq->GoToNextNoteonTick();
                    });
  dispatcher()->reg(this, "rewind",
                    [this]
                    {
                      if (auto seq = orchestrion()->sequencer())
                        seq->GoToTick(0, JumpReason::Rewind);
                    });

  // Repeats are unrolled at load time when grading is on (see
  // Orchestrion::init) — a one-way, in-place rewrite of the score. So a
  // grading switch only takes effect, in either direction, by reloading the
  // score.
  globalContext()->currentMasterNotationChanged().onNotify(
      this, [this]
      { m_scoreLoadedWithGrading = sequencerConfig()->gradingEnabled(); });
  sequencerConfig()->gradingEnabledChanged().onNotify(this, [this]
                                                      { reloadForGrading(); });

  projectConfiguration()->setShouldAskSaveLocationType(false);
  projectConfiguration()->setLastUsedSaveLocationType(
      mu::project::SaveLocationType::Local);
}

void OrchestrionActionController::reloadForGrading()
{
  const bool grading = sequencerConfig()->gradingEnabled();
  if (grading == m_scoreLoadedWithGrading)
    return; // the loaded score already has the repeat layout we need
  m_scoreLoadedWithGrading = grading;

  const mu::project::INotationProjectPtr project =
      globalContext()->currentProject();
  if (!project)
    return;
  const QUrl url = QUrl::fromLocalFile(project->path().toQString());
  if (!url.isValid())
    return;

  // Deferred: the switch usually comes from a click in the score view, whose
  // model we are about to tear down and rebuild.
  muse::async::Async::call(this, [this, url]
                           { openProject(mu::project::ProjectFile(url)); });
}

bool OrchestrionActionController::eventFilter(QObject *watched, QEvent *event)
{
  const bool mainWindowClosing =
      event->type() == QEvent::Close && watched == mainWindow()->qWindow();
  if (mainWindowClosing || event->type() == QEvent::Quit)
  {
    if (!closeCurrentProject())
    {
      // Cancel the close / quit
      event->ignore();
      return true;
    }
  }
  return QObject::eventFilter(watched, event);
}

bool OrchestrionActionController::closeCurrentProject() const
{
  if (!globalContext()->currentProject())
    return true;

  if (scoreModified())
  {
    using muse::IInteractive;
    const IInteractive::Result result = interactive()->questionSync(
        muse::trc("project", "Save changes to the score before closing?"),
        muse::trc("project",
                  "Your changes will be lost if you don't save them."),
        IInteractive::Buttons{IInteractive::Button::Save,
                              IInteractive::Button::Discard,
                              IInteractive::Button::Cancel},
        IInteractive::Button::Save);
    if (result.standardButton() == IInteractive::Button::Cancel)
      return false;
    if (result.standardButton() == IInteractive::Button::Save)
      onFileSave();
  }

  // Not through MuseScore's closeOpenedProject(): that asks whether to save
  // the "changes" of any imported (e.g. MusicXML) score, a question
  // Orchestrion, which does not edit scores, has no business asking. (On quit,
  // closing the project here rather than leaving it to the context teardown
  // also lets the services detach from the score while it is still alive;
  // tearing the context down with the project open destroyed the score while
  // the playback controller was still being notified of the change.)
  if (orchestrion()->player()->IsPlaying())
    dispatcher()->dispatch(actionIds::playbackStop);
  m_takeReplayer->Stop();
  offerTakes();
  interactive()->closeAllDialogsSync();
  globalContext()->setCurrentProject(nullptr);
  return true;
}

QString OrchestrionActionController::takesDirectory() const
{
  return globalConfiguration()->userAppDataPath().toQString() + "/takes";
}

void OrchestrionActionController::offerTakes() const
{
  const std::vector<Take> takes = m_takeRecorder->Takes();
  m_takeRecorder->Clear();
  if (takes.empty())
    return;

  QVariantList labels;
  for (size_t i = 0; i < takes.size(); ++i)
  {
    const Take &take = takes[i];
    const int seconds = static_cast<int>(take.DurationMs() / 1000.0 + 0.5);
    labels << muse::qtrc("orchestrion/takes",
                         "Take %1: from bar %2, %3 notes, %4:%5")
                  .arg(i + 1)
                  .arg(take.startBar)
                  .arg(take.NoteCount())
                  .arg(seconds / 60)
                  .arg(seconds % 60, 2, 10, QChar('0'));
  }
  muse::UriQuery query("orchestrion://takes/keep");
  query.addParam("takes", muse::Val::fromQVariant(labels));
  const muse::RetVal<muse::Val> kept = interactive()->openSync(query);
  if (!kept.ret)
    return;

  const QString directory = takesDirectory();
  QDir{}.mkpath(directory);
  const QString stem =
      QFileInfo{takes.front().scorePath}.completeBaseName() + " " +
      QDateTime::currentDateTime().toString("yyyy-MM-dd hh.mm.ss");
  for (const QVariant &index : kept.val.toQVariant().toList())
  {
    const auto i = index.toInt();
    if (i < 0 || i >= static_cast<int>(takes.size()))
      continue;
    QFile file{directory + "/" + stem + QString(" take %1.json").arg(i + 1)};
    if (file.open(QIODevice::WriteOnly))
      file.write(ToJson(takes[static_cast<size_t>(i)]));
    else
      LOGE() << "Could not save a take to " << file.fileName();
  }
}

void OrchestrionActionController::replayTake() const
{
  const muse::io::path_t path = interactive()->selectOpeningFileSync(
      muse::trc("orchestrion/takes", "Replay a take"),
      muse::io::path_t{takesDirectory()},
      {muse::trc("orchestrion/takes", "Takes") + " (*.json)"});
  if (path.empty())
    return;
  QFile file{path.toQString()};
  std::optional<Take> take;
  if (file.open(QIODevice::ReadOnly))
    take = TakeFromJson(file.readAll());
  if (!take)
  {
    LOGE() << "Not a take: " << path;
    return;
  }
  const auto project = globalContext()->currentProject();
  if (!project || project->path().toQString() != take->scorePath)
  {
    // The take's score opens first; the replay starts once it is ready.
    if (!closeCurrentProject())
      return;
    projectFilesController()->openProject(
        mu::project::ProjectFile{muse::io::path_t{take->scorePath}});
  }
  m_takeReplayer->Replay(std::move(*take));
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
    const QString displayNameOverride =
        args.count() >= 2 ? args.arg<QString>(1) : QString();
    const mu::project::ProjectFile projectFile(url, displayNameOverride);
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
    if (notationProject->save())
      markScoreSaved();
}

bool OrchestrionActionController::scoreModified() const
{
  // Orchestrion's own modifications (recorded velocities, ...) are not in the
  // score's undo stack. Its edits of the score (the staff modes, ...) are.
  // Not MuseScore's isNeedSave(), which any imported score starts with.
  const IModifiableItemRegistryPtr registry =
      orchestrion()->modifiableItemRegistry();
  if (registry && registry->Modified())
    return true;
  const auto master = globalContext()->currentMasterNotation();
  return master && !master->notation()->undoStack()->isStackClean();
}

void OrchestrionActionController::markScoreSaved() const
{
  // MuseScore doesn't move the clean state of the undo stack when it saves
  // (it keeps its own isNeedSave() flag): the stack is clean from here on.
  if (const auto master = globalContext()->currentMasterNotation())
    master->masterScore()->undoStack()->markClean();
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
  const muse::io::path_t filePath = interactive()->selectSavingFileSync(
      muse::trc("project", "Save as"), defaultDir, filter);
  if (projectFilesController()->saveProjectLocally(
          filePath, mu::project::SaveMode::SaveAs))
    markScoreSaved();
}

void OrchestrionActionController::openFromDir(const muse::io::path_t &dir) const
{
  const muse::io::path_t filePath = interactive()->selectOpeningFileSync(
      muse::trc("project", "Open"), dir, filter);

  if (filePath.empty())
    return;

  const mu::project::ProjectFile projectFile{filePath};
  openProject(projectFile);
}

void OrchestrionActionController::openProject(
    const mu::project::ProjectFile &projectFile) const
{
  // MuseScore opens a second window if a project is still open in this one.
  if (!closeCurrentProject())
    return;

  projectConfiguration()->setLastOpenedProjectsPath(projectFile.path());

  projectFilesController()->openProject(projectFile);
}

void OrchestrionActionController::toggleRecording() const
{
  sequencerConfig()->setVelocityRecordingEnabled(
      !sequencerConfig()->velocityRecordingEnabled());
}
} // namespace dgk