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
#include "OrchestrionCommands.h"
#include "MuseScoreShell/OrchestrionActionIds.h"

namespace dgk
{
namespace
{
constexpr auto moduleName_ = "orchestrion";

struct ActionCommand
{
  muse::rcommand::Command command;
  muse::actions::ActionCode action;
  muse::TranslatableString title;
};

const std::vector<ActionCommand> &actionCommands()
{
  static const std::vector<ActionCommand> commands{
      {muse::rcommand::Command("command://orchestrion/browse-scores"),
       actionIds::browseScores,
       muse::TranslatableString("orchestrion", "Browse scores")},
      {muse::rcommand::Command("command://orchestrion/file-save"),
       "orchestrion-file-save",
       muse::TranslatableString("orchestrion", "Save")},
      {muse::rcommand::Command("command://orchestrion/file-save-as"),
       "orchestrion-file-save-as",
       muse::TranslatableString("orchestrion", "Save as")},
      {muse::rcommand::Command("command://orchestrion/play-toggle"),
       actionIds::playbackToggle,
       muse::TranslatableString("orchestrion", "Play")},
      {muse::rcommand::Command("command://orchestrion/rewind"), "rewind",
       muse::TranslatableString("orchestrion", "Rewind")},
      {muse::rcommand::Command("command://orchestrion/prev"), "prev",
       muse::TranslatableString("orchestrion", "Previous chord")},
      {muse::rcommand::Command("command://orchestrion/next"), "next",
       muse::TranslatableString("orchestrion", "Next chord")},
      {muse::rcommand::Command("command://orchestrion/loop-in"),
       actionIds::loopIn, muse::TranslatableString("orchestrion", "Loop in")},
      {muse::rcommand::Command("command://orchestrion/loop-out"),
       actionIds::loopOut, muse::TranslatableString("orchestrion", "Loop out")},
      {muse::rcommand::Command("command://orchestrion/fullscreen"),
       "view-toggle-fullscreen",
       muse::TranslatableString("orchestrion", "Full screen")},
  };
  return commands;
}
} // namespace

std::string OrchestrionCommandsRegister::moduleName() const
{
  return moduleName_;
}

const std::vector<muse::rcommand::Command> &
OrchestrionCommandsRegister::commandList() const
{
  static const std::vector<muse::rcommand::Command> list = []
  {
    std::vector<muse::rcommand::Command> list;
    for (const ActionCommand &ac : actionCommands())
      list.push_back(ac.command);
    return list;
  }();
  return list;
}

const std::vector<muse::rcommand::CommandInfo> &
OrchestrionCommandsRegister::commandInfoList() const
{
  static const std::vector<muse::rcommand::CommandInfo> list = []
  {
    std::vector<muse::rcommand::CommandInfo> list;
    for (const ActionCommand &ac : actionCommands())
      list.emplace_back(ac.command, ac.title, ac.title,
                        muse::rcommand::InputSchema(),
                        muse::rcommand::Decoration());
    return list;
  }();
  return list;
}

void OrchestrionCommands::registerHandlers()
{
  for (const ActionCommand &ac : actionCommands())
    commandDispatcher()->onRequest(this, ac.command,
                                   [this, action = ac.action]
                                   {
                                     actionsDispatcher()->dispatch(action);
                                     return muse::make_ok();
                                   });
}

std::string OrchestrionCommands::moduleName() const { return moduleName_; }

muse::rcommand::CommandState
OrchestrionCommands::commandState(const muse::rcommand::Command &) const
{
  return muse::rcommand::CommandState(true);
}

muse::async::Channel<muse::rcommand::Command, muse::rcommand::CommandState>
OrchestrionCommands::commandStateChanged() const
{
  return m_commandStateChanged;
}
} // namespace dgk
