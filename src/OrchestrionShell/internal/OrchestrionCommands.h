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
#pragma once

#include "OrchestrionCommon/OrchestrionIoc.h"

#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <rcommand/commandable.h>
#include <rcommand/icommanddispatcher.h>
#include <rcommand/imodulecommandsregister.h>
#include <rcommand/imodulecommandsstate.h>

namespace dgk
{
/**
 * The commands that carry Orchestrion's keyboard shortcuts.
 *
 * MuseScore's shortcuts (v2) only trigger commands, and Orchestrion's
 * handlers are still actions: each of these commands dispatches the action of
 * the same name. A command whose handler moves off actions just changes its
 * registration in OrchestrionCommands.
 */
class OrchestrionCommandsRegister
    : public muse::rcommand::IModuleCommandsRegister
{
public:
  std::string moduleName() const override;
  const std::vector<muse::rcommand::Command> &commandList() const override;
  const std::vector<muse::rcommand::CommandInfo> &
  commandInfoList() const override;
};

/**
 * The state (always enabled, like the actions behind them) and the handlers
 * of the commands of OrchestrionCommandsRegister.
 */
class OrchestrionCommands : public muse::rcommand::IModuleCommandsState,
                            public muse::rcommand::Commandable,
                            public muse::async::Asyncable,
                            public dgk::Injectable
{
  dgk::Inject<muse::rcommand::ICommandDispatcher> commandDispatcher{this};
  dgk::Inject<muse::actions::IActionsDispatcher> actionsDispatcher{this};

public:
  /**
   * Registers the handlers; call once the context's dispatchers exist.
   */
  void registerHandlers();

  // IModuleCommandsState
  std::string moduleName() const override;
  void init() override {}
  void deinit() override {}
  muse::rcommand::CommandState
  commandState(const muse::rcommand::Command &) const override;
  muse::async::Channel<muse::rcommand::Command, muse::rcommand::CommandState>
  commandStateChanged() const override;

private:
  muse::async::Channel<muse::rcommand::Command, muse::rcommand::CommandState>
      m_commandStateChanged;
};
} // namespace dgk
