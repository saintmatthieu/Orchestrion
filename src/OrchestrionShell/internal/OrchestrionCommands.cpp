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

#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>

namespace dgk
{
namespace
{
constexpr auto moduleName_ = "orchestrion";

const muse::rcommand::Command
    openScoreCommand("command://orchestrion/open-score");

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
    std::vector<muse::rcommand::Command> list{openScoreCommand};
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
    list.emplace_back(
        openScoreCommand, muse::TranslatableString("orchestrion", "Open score"),
        muse::TranslatableString("orchestrion", "Open a score"),
        muse::rcommand::InputSchema(
            {{"path", muse::rcommand::Arg(
                          muse::rcommand::DataType::String,
                          u"Absolute, or relative to the bundled scores")}}),
        muse::rcommand::Decoration());
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
  onRequest(openScoreCommand, [this](const muse::rcommand::Params &params)
            { return openScore(params); });
  for (const ActionCommand &ac : actionCommands())
    onRequest(ac.command,
              [this, action = ac.action](const muse::rcommand::Params &)
              {
                actionsDispatcher()->dispatch(action);
                return muse::make_ok();
              });
}

void OrchestrionCommands::onRequest(const muse::rcommand::Command &command,
                                    const Handler &handler)
{
  commandDispatcher()->onRequest(
      this, command,
      muse::rcommand::ICommandDispatcher::AsyncCallBack(
          [handler](
              const muse::rcommand::Request &request,
              const muse::rcommand::ICommandDispatcher::OnResponse &onResponse)
          {
            QMetaObject::invokeMethod(
                qApp,
                [handler, request, onResponse]
                {
                  onResponse(muse::rcommand::make_response(
                      request, handler(request.params)));
                },
                Qt::QueuedConnection);
          }));
}

muse::Ret OrchestrionCommands::openScore(const muse::rcommand::Params &params)
{
  const auto it = params.find("path");
  const QString arg = it == params.end() ? QString() : it->second.toQString();
  const QString path =
      QFileInfo{arg}.isAbsolute()
          ? arg
          : (globalConfiguration()->appDataPath() + "scores/" + arg)
                .toQString();
  if (!QFileInfo::exists(path))
    return muse::make_ret(muse::Ret::Code::UnknownError,
                          "no such score: " + path.toStdString());

  actionsDispatcher()->dispatch(
      "orchestrion-file-open",
      muse::actions::ActionData::make_arg2<QUrl, QString>(
          QUrl::fromLocalFile(path), QString{}));

  // Opening is synchronous, but may have been refused (by the user, asked
  // about the current score's changes) or have failed.
  const auto project = projectSession()->project();
  if (!project || QFileInfo{project->path().toQString()}.absoluteFilePath() !=
                      QFileInfo{path}.absoluteFilePath())
    return muse::make_ret(muse::Ret::Code::UnknownError,
                          "score not opened: " + path.toStdString());
  return muse::make_ok();
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
