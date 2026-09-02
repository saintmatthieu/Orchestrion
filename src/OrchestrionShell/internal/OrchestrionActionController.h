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
#include "OrchestrionSynthesis/IOrchestrionSynthesisConfiguration.h"

#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <context/iglobalcontext.h>
#include <global/iglobalconfiguration.h>
#include <interactive/iinteractive.h>
#include <interactive/iplatforminteractive.h>
#include <modularity/ioc.h>
#include <project/iprojectconfiguration.h>
#include <project/iprojectfilescontroller.h>
#include <ui/imainwindow.h>

#include <QObject>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
class OrchestrionActionController : public muse::actions::Actionable,
                                    public dgk::Injectable,
                                    public muse::async::Asyncable,
                                    public QObject
{
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfig{this};
  dgk::Inject<IOrchestrionSynthesisConfiguration> synthesisConfig{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};
  dgk::Inject<mu::project::IProjectConfiguration> projectConfiguration{this};
  dgk::Inject<mu::project::IProjectFilesController> projectFilesController{this};
  dgk::Inject<muse::IInteractive> interactive{this};
  dgk::Inject<muse::IPlatformInteractive> platformInteractive{this};
  dgk::Inject<muse::ui::IMainWindow> mainWindow{this};

public:
  void preInit();
  void init();

private:
  bool eventFilter(QObject *obj, QEvent *event) override;

  void onFileOpen(const muse::actions::ActionData &data) const;
  void onFileSave() const;
  void onFileSaveAs() const;
  void openFromDir(const muse::io::path_t &dir) const;
  void openProject(const mu::project::ProjectFile &) const;
  void toggleRecording() const;
  muse::io::path_t fallbackPath() const;
  //! Grading needs the score's repeats unrolled and plain playing needs them
  //! intact; unrolling rewrites the loaded score in place, so switching
  //! grading means reloading the score.
  void reloadForGrading();

  //! Whether grading was on when the current score was loaded — i.e. whether
  //! its repeats were unrolled.
  bool m_scoreLoadedWithGrading = false;
};
} // namespace dgk