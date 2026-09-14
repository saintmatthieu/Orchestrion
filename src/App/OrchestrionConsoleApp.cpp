/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "OrchestrionConsoleApp.h"
#include <audioplugins/iregisteraudiopluginsscenario.h>
#include <log.h>
#include <QCoreApplication>
#include <iostream>

namespace dgk
{
OrchestrionConsoleApp::OrchestrionConsoleApp(
    const std::shared_ptr<CommandOptions> &options)
    : muse::ConsoleApplication(options)
{
}

void OrchestrionConsoleApp::showSplash()
{
  std::cout << "The Orchestrion console application is starting..."
            << std::endl;
}

void OrchestrionConsoleApp::doStartupScenario(
    const muse::modularity::ContextPtr &ctx)
{
  const std::shared_ptr<CommandOptions> options =
      std::dynamic_pointer_cast<CommandOptions>(contextData(ctx).options);
  IF_ASSERT_FAILED(options)
  {
    qApp->exit(EXIT_FAILURE);
    return;
  }

  int code = EXIT_FAILURE;
  switch (options->runMode)
  {
  case muse::IApplication::RunMode::AudioPluginRegistration:
    code = registerAudioPlugin(options->audioPluginRegistration, ctx);
    break;
  default:
    LOGE() << "Unsupported console run mode";
    break;
  }
  qApp->exit(code);
}

int OrchestrionConsoleApp::registerAudioPlugin(
    const CommandOptions::AudioPluginRegistration &task,
    const muse::modularity::ContextPtr &ctx)
{
  muse::ContextInject<muse::audioplugins::IRegisterAudioPluginsScenario>
      registerAudioPluginsScenario{ctx};
  const muse::Ret ret =
      task.outputFile.empty()
          ? registerAudioPluginsScenario()->registerPlugin(task.pluginPath)
          : registerAudioPluginsScenario()->validatePlugin(task.pluginPath,
                                                           task.outputFile);
  if (!ret)
    LOGE() << ret.toString();
  return ret.code();
}
} // namespace dgk
