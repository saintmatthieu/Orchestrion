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
#include "OrchestrionShellModule.h"
#include "internal/MidiControllerMenuManager.h"
#include "internal/OrchestrionStartupScenario.h"
#include "internal/OrchestrionUiActions.h"
#include "internal/PlaybackDeviceMenuManager.h"
#include "modularity/ioc.h"
#include "ui/iuiactionsregister.h"
#include "view/NotationPaintViewLoaderModel.h"
#include "view/OrchestrionWindowTitleProvider.h"
#include <QQmlEngine>

namespace dgk::orchestrion
{
OrchestrionShellModule::OrchestrionShellModule()
    : m_midiControllerMenuManager{std::make_shared<
          MidiControllerMenuManager>()},
      m_playbackDeviceMenuManager{
          std::make_shared<PlaybackDeviceMenuManager>()},
      m_orchestrionUiActions{std::make_shared<OrchestrionUiActions>(
          m_midiControllerMenuManager, m_playbackDeviceMenuManager)}
{
}

std::string OrchestrionShellModule::moduleName() const
{
  return "OrchestrionShell";
}

void OrchestrionShellModule::registerExports()
{
  ioc()->registerExport<IOrchestrionUiActions>(moduleName(),
                                               m_orchestrionUiActions);
  ioc()->registerExport<IOrchestrionStartupScenario>(
      moduleName(), std::make_shared<OrchestrionStartupScenario>());
  ioc()->registerExport<IMidiControllerManager>(moduleName(),
                                                m_midiControllerMenuManager);
}

void OrchestrionShellModule::resolveImports()
{
  auto ar = ioc()->resolve<muse::ui::IUiActionsRegister>(moduleName());
  if (ar)
    ar->reg(m_orchestrionUiActions);
}

void OrchestrionShellModule::registerResources()
{
  Q_INIT_RESOURCE(appshell);
  Q_INIT_RESOURCE(OrchestrionShell);
}

void OrchestrionShellModule::registerUiTypes()
{
  qmlRegisterType<NotationPaintViewLoaderModel>(
      "Orchestrion.OrchestrionShell", 1, 0, "NotationPaintViewLoaderModel");
  qmlRegisterType<OrchestrionWindowTitleProvider>(
      "Orchestrion.OrchestrionShell", 1, 0, "OrchestrionWindowTitleProvider");
}

void OrchestrionShellModule::onPreInit(const muse::IApplication::RunMode &mode)
{
  m_playbackDeviceMenuManager->init();
}

void OrchestrionShellModule::onInit(const muse::IApplication::RunMode &mode)
{
  if (mode == muse::IApplication::RunMode::AudioPluginRegistration)
    return;

  m_orchestrionUiActions->init();
}
} // namespace dgk::orchestrion
