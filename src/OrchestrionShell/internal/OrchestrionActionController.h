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

#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <context/iglobalcontext.h>
#include <global/iglobalconfiguration.h>
#include <global/iinteractive.h>
#include <modularity/ioc.h>
#include <project/iprojectconfiguration.h>
#include <project/iprojectfilescontroller.h>
#include <ui/imainwindow.h>

#include <QObject>

namespace dgk
{
class OrchestrionActionController : public muse::actions::Actionable,
                                    public muse::Injectable,
                                    public QObject
{
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<IOrchestrionSequencerConfiguration> sequencerConfig;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<muse::IGlobalConfiguration> globalConfiguration;
  muse::Inject<mu::project::IProjectConfiguration> projectConfiguration;
  muse::Inject<mu::project::IProjectFilesController> projectFilesController;
  muse::Inject<muse::IInteractive> interactive;
  muse::Inject<muse::ui::IMainWindow> mainWindow;

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
};
} // namespace dgk