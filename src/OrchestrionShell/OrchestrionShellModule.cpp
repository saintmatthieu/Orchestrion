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
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "interactive/iinteractiveuriregister.h"
#include "internal/ControllerMenuManager.h"
#include "internal/OrchestrionActionController.h"
#include "internal/OrchestrionEventProcessor.h"
#include "internal/OrchestrionStartupScenario.h"
#include "internal/OrchestrionUiActions.h"
#include "internal/PlaybackDeviceMenuManager.h"
#include "internal/SleepInhibitor.h"
#include "internal/SynthesizerMenuManager.h"
#include "modularity/ioc.h"
#include "ui/iuiactionsregister.h"
#include "view/AutoPlayModel.h"
#include "view/GradingModel.h"
#ifdef Q_OS_MAC
#include "view/MacOSWindowChrome.h"
#endif
#include "view/MidiKeyboardIconModel.h"
#include "view/NotationPaintViewLoaderModel.h"
#include "view/PlaybackButtonModel.h"
#include "view/ScoreAttributionModel.h"
#include "view/ScoreHeadingModel.h"

#include <QQmlEngine>

namespace dgk
{
OrchestrionShellModule::OrchestrionShellModule()
    : m_midiControllerMenuManager{std::make_shared<ControllerMenuManager>()},
      m_midiSynthesizerMenuManager{std::make_shared<SynthesizerMenuManager>()},
      m_playbackDeviceMenuManager{
          std::make_shared<PlaybackDeviceMenuManager>()},
      m_orchestrionEventProcessor{
          std::make_shared<OrchestrionEventProcessor>()},
      m_orchestrionUiActions{std::make_shared<OrchestrionUiActions>(
          m_midiControllerMenuManager, m_midiSynthesizerMenuManager,
          m_playbackDeviceMenuManager)},
      m_orchestrionActionController{
          std::make_shared<OrchestrionActionController>()},
      m_orchestrionStartupScenario{
          std::make_shared<OrchestrionStartupScenario>()},
      m_sleepInhibitor{std::make_shared<SleepInhibitor>()}
{
}

std::string OrchestrionShellModule::moduleName() const
{
  return "OrchestrionShell";
}

void OrchestrionShellModule::registerExports()
{
  globalIoc()->registerExport<IOrchestrionUiActions>(moduleName(),
                                                     m_orchestrionUiActions);
  globalIoc()->registerExport<IOrchestrionStartupScenario>(
      moduleName(), m_orchestrionStartupScenario);
}

void OrchestrionShellModule::registerUiTypes()
{
  qmlRegisterType<NotationPaintViewLoaderModel>(
      "Orchestrion.OrchestrionShell", 1, 0, "NotationPaintViewLoaderModel");
  qmlRegisterType<ScoreHeadingModel>("Orchestrion.OrchestrionShell", 1, 0,
                                     "ScoreHeadingModel");
  qmlRegisterType<PlaybackButtonModel>("Orchestrion.OrchestrionShell", 1, 0,
                                       "PlaybackButtonModel");
  qmlRegisterType<GradingModel>("Orchestrion.OrchestrionShell", 1, 0,
                                "GradingModel");
  qmlRegisterType<MidiKeyboardIconModel>("Orchestrion.OrchestrionShell", 1, 0,
                                         "MidiKeyboardIconModel");
  qmlRegisterType<AutoPlayModel>("Orchestrion.OrchestrionShell", 1, 0,
                                 "AutoPlayModel");
  qmlRegisterType<ScoreAttributionModel>("Orchestrion.OrchestrionShell", 1, 0,
                                         "ScoreAttributionModel");
#ifdef Q_OS_MAC
  qmlRegisterType<MacOSWindowChrome>("Orchestrion.OrchestrionShell", 1, 0,
                                     "MacOSWindowChrome");
#endif
}

void OrchestrionShellModule::resolveImports()
{
  // MuseScore's standard dialogs (question, info, warning, error) are replaced
  // with Orchestrion's, which draw their own title bar on Windows and Linux
  // (see OrchestrionStandardDialog.qml). Unregistered first: the register
  // asserts on a duplicate.
  auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(
      moduleName());
  if (ir)
  {
    const muse::Uri uri("muse://interactive/standard");
    ir->unregisterUri(uri);
    ir->registerQmlUri(uri, "Orchestrion", "OrchestrionStandardDialog");
  }
}

muse::modularity::IContextSetup *OrchestrionShellModule::newContext(
    const muse::modularity::ContextPtr &ctx) const
{
  ModuleContextSetup::Hooks hooks;
  hooks.resolveImports = [this] { onContextResolveImports(); };
  hooks.onPreInit = [this](const muse::IApplication::RunMode &mode)
  { onContextPreInit(mode); };
  hooks.onInit = [this](const muse::IApplication::RunMode &mode)
  { onContextInit(mode); };
  return new ModuleContextSetup(ctx, std::move(hooks));
}

void OrchestrionShellModule::onContextResolveImports() const
{
  // The UI actions register is context-scoped.
  auto ar = muse::modularity::ioc(iocContext())
                ->resolve<muse::ui::IUiActionsRegister>(moduleName());
  if (ar)
    ar->reg(m_orchestrionUiActions);
}

void OrchestrionShellModule::onContextPreInit(
    const muse::IApplication::RunMode &) const
{
  m_orchestrionActionController->preInit();
}

void OrchestrionShellModule::onContextInit(
    const muse::IApplication::RunMode &mode) const
{
  if (mode == muse::IApplication::RunMode::AudioPluginRegistration)
    return;
  m_orchestrionEventProcessor->init();
  m_orchestrionUiActions->init();
  m_orchestrionActionController->init();
  m_orchestrionStartupScenario->init();
  m_sleepInhibitor->init();
}
} // namespace dgk
