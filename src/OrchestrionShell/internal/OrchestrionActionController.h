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
#pragma once

#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSynthesis/IEffectChain.h"

#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <global/iapplication.h>
#include <global/iglobalconfiguration.h>
#include <interactive/iinteractive.h>
#include <interactive/iplatforminteractive.h>
#include <modularity/ioc.h>
#include <multiwindows/imultiwindowsprovider.h>
#include <project/iopenprojectscenario.h>
#include <project/iprojectautosaver.h>
#include <project/iprojectconfiguration.h>
#include <project/iprojectcreator.h>
#include <project/iprojectsession.h>
#include <project/irecentfilescontroller.h>
#include <project/isaveprojectscenario.h>
#include <ui/imainwindow.h>
#include <ui/iuiconfiguration.h>

#include <QObject>
#include <QString>
#include <memory>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
class TakeRecorder;
class TakeReplayer;

class OrchestrionActionController : public muse::actions::Actionable,
                                    public dgk::Injectable,
                                    public muse::async::Asyncable,
                                    public QObject
{
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfig{this};
  dgk::Inject<IEffectChain> effectChain{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<mu::project::IProjectSession> projectSession{this};
  dgk::Inject<muse::IApplication> application{this};
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};
  dgk::Inject<mu::project::IProjectConfiguration> projectConfiguration{this};
  dgk::Inject<mu::project::IOpenProjectScenario> openProjectScenario{this};
  dgk::Inject<mu::project::ISaveProjectScenario> saveProjectScenario{this};
  dgk::Inject<mu::project::IRecentFilesController> recentFilesController{this};
  dgk::Inject<mu::project::IProjectCreator> projectCreator{this};
  dgk::Inject<mu::project::IProjectAutoSaver> projectAutoSaver{this};
  dgk::Inject<muse::mi::IMultiWindowsProvider> multiwindowsProvider{this};
  dgk::Inject<muse::IInteractive> interactive{this};
  dgk::Inject<muse::IPlatformInteractive> platformInteractive{this};
  dgk::Inject<muse::ui::IMainWindow> mainWindow{this};
  dgk::Inject<muse::ui::IUiConfiguration> uiConfiguration{this};

public:
  OrchestrionActionController();
  ~OrchestrionActionController();
  void preInit();
  void init();

private:
  bool eventFilter(QObject *obj, QEvent *event) override;
  /**
   * Closes the current project, if any, asking about Orchestrion's unsaved
   * modifications (not about MuseScore's notion of unsaved changes, which
   * counts every imported score). Used before opening another score and
   * before quitting. Returns false if the user cancelled.
   */
  bool closeCurrentProject() const;

  void onFileOpen(const muse::actions::ActionData &data) const;
  void onFileSave() const;
  void onFileSaveAs();
  /**
   * Whether the score has changes to save: Orchestrion's own modifications,
   * or edits in the score's undo stack.
   */
  bool scoreModified() const;
  /** Gets Orchestrion's modifications and the score ready to be written. */
  void prepareSave() const;
  /** The score was just saved: its undo stack is clean from here on. */
  void markScoreSaved() const;
  void openFromDir(const muse::io::path_t &dir) const;
  void openProject(const mu::project::ProjectFile &) const;
  /**
   * Opens a score on this computer without MuseScore's sanity check, which
   * would otherwise ask whether to open a "corrupted" file. Returns false if
   * it couldn't, for MuseScore's own opening to try (and explain why not).
   */
  bool openLocalProjectForced(const mu::project::ProjectFile &) const;
  void toggleRecording() const;
  /** Asks which of the session's takes to keep, and saves them. */
  void offerTakes() const;
  void replayTake() const;
  /**
   * After asking, forgets every setting (the effects' and the shortcuts'
   * included), the recent scores and the folders added to the shelf, and
   * restarts. The scores, those downloaded from Rolls included, and the takes
   * are kept.
   */
  void revertToFactorySettings() const;
  /** Where the takes are saved. */
  QString takesDirectory() const;
  muse::io::path_t fallbackPath() const;
  //! Grading needs the score's repeats unrolled and plain playing needs them
  //! intact; unrolling rewrites the loaded score in place, so switching
  //! grading means reloading the score.
  void reloadForGrading();

  //! Whether grading was on when the current score was loaded — i.e. whether
  //! its repeats were unrolled.
  bool m_scoreLoadedWithGrading = false;

  const std::unique_ptr<TakeRecorder> m_takeRecorder;
  const std::unique_ptr<TakeReplayer> m_takeReplayer;
};
} // namespace dgk