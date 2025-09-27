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

#include "CommandOptions.h"
#include "OrchestrionShell/IOrchestrionStartupScenario.h"
#include <global/iapplication.h>
#include <global/modularity/imodulesetup.h>
#include <global/modularity/ioc.h>
#include <project/iprojectconfiguration.h>

namespace dgk
{
class OrchestrionApp : public muse::IApplication,
                       public std::enable_shared_from_this<OrchestrionApp>
{
  INJECT(mu::project::IProjectConfiguration, projectConfiguration);
  INJECT(IOrchestrionStartupScenario, startupScenario);

public:
  OrchestrionApp(CommandOptions options);

  void addModule(muse::modularity::IModuleSetup *module);

  muse::String name() const override { return muse::String{"Orchestrion"}; }
  muse::String title() const override { return muse::String{"Orchestrion"}; }
  bool unstable() const override { return true; }
  muse::Version version() const override { return muse::Version(0, 0, 0); }
  muse::Version fullVersion() const override { return muse::Version(0, 0, 0); }
  muse::String build() const override { return muse::String{"0"}; }
  muse::String revision() const override { return muse::String{"0"}; }
  RunMode runMode() const override { return RunMode::GuiApp; }
  bool noGui() const override { return false; }
  void perform() override;
  void finish() override {}
  void restart() override {}
  muse::modularity::ModulesIoC *ioc() const override;
  const muse::modularity::ContextPtr iocContext() const override;
  QWindow *focusWindow() const override;
  bool notify(QObject *, QEvent *) override;
  Qt::KeyboardModifiers keyboardModifiers() const override;

private:
  const CommandOptions m_opts;
  const std::shared_ptr<muse::modularity::Context> m_context;
  QList<muse::modularity::IModuleSetup *> m_modules;
};
} // namespace dgk
