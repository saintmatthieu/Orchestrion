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

#include <notation/imasternotation.h>
#include <notation/inotation.h>
#include <notation/inotationundostack.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QTimer>
#include <QUrl>

namespace dgk
{
namespace
{
constexpr auto moduleName_ = "orchestrion";

const muse::rcommand::Command
    openScoreCommand("command://orchestrion/open-score");
const muse::rcommand::Command quitCommand("command://orchestrion/quit");
// The demo cursor (DemoCursor): `control` is a navigation path, e.g.
// Controls/Playback/Play.
const muse::rcommand::Command
    demoClickCommand("command://orchestrion/demo-click");
const muse::rcommand::Command
    demoMoveCommand("command://orchestrion/demo-move");
const muse::rcommand::Command
    demoHideCommand("command://orchestrion/demo-hide");

// The demo's keys and caption (DemoKeys)
const muse::rcommand::Command
    demoKeysCommand("command://orchestrion/demo-keys");
const muse::rcommand::Command
    demoCaptionCommand("command://orchestrion/demo-caption");
const muse::rcommand::Command
    demoKeyPressCommand("command://orchestrion/demo-key-press");
const muse::rcommand::Command
    demoKeyReleaseCommand("command://orchestrion/demo-key-release");
// Answers once the player has stopped, e.g. at the end of the score
const muse::rcommand::Command
    waitUntilStoppedCommand("command://orchestrion/wait-until-stopped");
const muse::rcommand::Command
    staffModeCommand("command://orchestrion/staff-mode");
const muse::rcommand::Command undoCommand("command://orchestrion/undo");
const muse::rcommand::Command
    midiNoteCommand("command://orchestrion/midi-note");
const muse::rcommand::Command
    midiPedalCommand("command://orchestrion/midi-pedal");
const muse::rcommand::Command
    midiDeviceCommand("command://orchestrion/midi-device");

muse::Ret virtualKeyboardInUseRet(bool inUse)
{
  return inUse ? muse::make_ok()
               : muse::make_ret(muse::Ret::Code::UnknownError,
                                "the virtual keyboard isn't the MIDI device "
                                "in use (see midi-device)");
}

/**
 * As the score names them (see StaffModes).
 */
std::string staffModeName(StaffMode mode)
{
  switch (mode)
  {
  case StaffMode::notes:
    return "notes";
  case StaffMode::rhythm:
    return "rhythm";
  case StaffMode::follow:
    return "follow";
  case StaffMode::mute:
    return "mute";
  }
  return "rhythm";
}

QString stringParam(const muse::rcommand::Params &params, const char *name)
{
  const auto it = params.find(name);
  return it == params.end() ? QString() : it->second.toQString();
}

QString controlParam(const muse::rcommand::Params &params)
{
  return stringParam(params, "control");
}

muse::Ret keyParamRet(const muse::rcommand::Params &params, bool sent)
{
  return sent ? muse::make_ok()
              : muse::make_ret(muse::Ret::Code::UnknownError,
                               "not a number key: " +
                                   stringParam(params, "key").toStdString());
}

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
      {muse::rcommand::Command("command://orchestrion/tutorials"),
       "orchestrion-tutorials",
       muse::TranslatableString("orchestrion", "Tutorials")},
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
    std::vector<muse::rcommand::Command> list{
        openScoreCommand,        quitCommand,         demoClickCommand,
        demoMoveCommand,         demoHideCommand,     demoKeysCommand,
        demoCaptionCommand,      demoKeyPressCommand, demoKeyReleaseCommand,
        waitUntilStoppedCommand, staffModeCommand,    undoCommand,
        midiNoteCommand,         midiPedalCommand,    midiDeviceCommand};
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
    const muse::rcommand::InputSchema controlSchema(
        {{"control", muse::rcommand::Arg(
                         muse::rcommand::DataType::String,
                         u"Navigation path, e.g. Controls/Playback/Play")}});
    list.emplace_back(
        demoClickCommand,
        muse::TranslatableString("orchestrion", "Demo: click"),
        muse::TranslatableString("orchestrion",
                                 "The demo cursor glides to a control and "
                                 "clicks it"),
        controlSchema, muse::rcommand::Decoration());
    list.emplace_back(demoMoveCommand,
                      muse::TranslatableString("orchestrion", "Demo: move"),
                      muse::TranslatableString(
                          "orchestrion", "The demo cursor glides to a control"),
                      controlSchema, muse::rcommand::Decoration());
    list.emplace_back(
        demoHideCommand, muse::TranslatableString("orchestrion", "Demo: hide"),
        muse::TranslatableString("orchestrion", "Hides the demo cursor"),
        muse::rcommand::InputSchema(), muse::rcommand::Decoration());
    const muse::rcommand::InputSchema keySchema(
        {{"key", muse::rcommand::Arg(muse::rcommand::DataType::String,
                                     u"A number key, 0 to 9")}});
    list.emplace_back(
        demoKeysCommand,
        muse::TranslatableString("orchestrion", "Demo: number keys"),
        muse::TranslatableString("orchestrion",
                                 "Shows or hides the number keys"),
        muse::rcommand::InputSchema(
            {{"shown", muse::rcommand::Arg(muse::rcommand::DataType::Boolean,
                                           u"Whether they show")}}),
        muse::rcommand::Decoration());
    list.emplace_back(
        demoCaptionCommand,
        muse::TranslatableString("orchestrion", "Demo: caption"),
        muse::TranslatableString("orchestrion",
                                 "Shows a caption; an empty one hides it"),
        muse::rcommand::InputSchema(
            {{"text", muse::rcommand::Arg(muse::rcommand::DataType::String,
                                          u"The caption")}}),
        muse::rcommand::Decoration());
    list.emplace_back(
        demoKeyPressCommand,
        muse::TranslatableString("orchestrion", "Demo: press key"),
        muse::TranslatableString("orchestrion", "Presses a number key"),
        keySchema, muse::rcommand::Decoration());
    list.emplace_back(
        demoKeyReleaseCommand,
        muse::TranslatableString("orchestrion", "Demo: release key"),
        muse::TranslatableString("orchestrion", "Releases a number key"),
        keySchema, muse::rcommand::Decoration());
    list.emplace_back(
        staffModeCommand, muse::TranslatableString("orchestrion", "Staff mode"),
        muse::TranslatableString("orchestrion",
                                 "Answers with how a staff is played: notes, "
                                 "rhythm, follow or mute"),
        muse::rcommand::InputSchema(
            {{"staff",
              muse::rcommand::Arg(muse::rcommand::DataType::Integer,
                                  u"0 the upper staff, 1 the lower")}}),
        muse::rcommand::Decoration());
    list.emplace_back(
        midiNoteCommand, muse::TranslatableString("orchestrion", "MIDI note"),
        muse::TranslatableString("orchestrion",
                                 "Plays a key of the virtual MIDI keyboard"),
        muse::rcommand::InputSchema(
            {{"pitch", muse::rcommand::Arg(muse::rcommand::DataType::Integer,
                                           u"The MIDI pitch, 60 middle C")},
             {"velocity",
              muse::rcommand::Arg(muse::rcommand::DataType::Float,
                                  u"How hard, 0 to 1; 0 releases the key")}}),
        muse::rcommand::Decoration());
    list.emplace_back(
        midiPedalCommand, muse::TranslatableString("orchestrion", "MIDI pedal"),
        muse::TranslatableString(
            "orchestrion",
            "Presses or lifts the virtual MIDI keyboard's pedal"),
        muse::rcommand::InputSchema(
            {{"down", muse::rcommand::Arg(muse::rcommand::DataType::Boolean,
                                          u"Whether it is down")}}),
        muse::rcommand::Decoration());
    list.emplace_back(
        midiDeviceCommand,
        muse::TranslatableString("orchestrion", "MIDI device"),
        muse::TranslatableString(
            "orchestrion",
            "Chooses the MIDI input device, as the Audio/MIDI menu does; "
            "answers with the one in use"),
        muse::rcommand::InputSchema(
            {{"id", muse::rcommand::Arg(
                        muse::rcommand::DataType::String,
                        u"Its id, \"virtual\" the virtual keyboard; none, "
                        u"no change")}}),
        muse::rcommand::Decoration());
    list.emplace_back(
        undoCommand, muse::TranslatableString("orchestrion", "Undo"),
        muse::TranslatableString("orchestrion",
                                 "Undoes the last edit of the score (the "
                                 "staves' modes, ...)"),
        muse::rcommand::InputSchema(), muse::rcommand::Decoration());
    list.emplace_back(
        waitUntilStoppedCommand,
        muse::TranslatableString("orchestrion", "Wait until stopped"),
        muse::TranslatableString("orchestrion",
                                 "Answers once the player has stopped"),
        muse::rcommand::InputSchema(), muse::rcommand::Decoration());
    list.emplace_back(
        quitCommand, muse::TranslatableString("orchestrion", "Quit"),
        muse::TranslatableString("orchestrion", "Quit Orchestrion"),
        muse::rcommand::InputSchema(), muse::rcommand::Decoration());
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
  onRequestAsync(demoClickCommand,
                 [this](const muse::rcommand::Params &params, Done done) {
                   m_demoCursor->click(controlParam(params), std::move(done));
                 });
  onRequestAsync(demoMoveCommand,
                 [this](const muse::rcommand::Params &params, Done done) {
                   m_demoCursor->moveTo(controlParam(params), std::move(done));
                 });
  onRequest(demoKeysCommand,
            [this](const muse::rcommand::Params &params)
            {
              const auto it = params.find("shown");
              m_demoKeys->setShown(it == params.end() || it->second.toBool());
              return muse::make_ok();
            });
  onRequest(demoCaptionCommand,
            [this](const muse::rcommand::Params &params)
            {
              m_demoKeys->setCaption(stringParam(params, "text"));
              return muse::make_ok();
            });
  onRequest(demoKeyPressCommand,
            [this](const muse::rcommand::Params &params)
            {
              const QString key = stringParam(params, "key");
              return keyParamRet(params,
                                 key.size() == 1 && m_demoKeys->press(key[0]));
            });
  onRequest(demoKeyReleaseCommand,
            [this](const muse::rcommand::Params &params)
            {
              const QString key = stringParam(params, "key");
              return keyParamRet(params, key.size() == 1 &&
                                             m_demoKeys->release(key[0]));
            });
  onRequest(staffModeCommand,
            [this](const muse::rcommand::Params &params)
            {
              const auto it = params.find("staff");
              muse::Ret ret = muse::make_ok();
              ret.setText(staffModeName(staffModes()->mode(
                  it == params.end() ? 0 : it->second.toInt())));
              return ret;
            });
  // A tutorial takes back what it changed of the user's score: undone, an
  // edit of a score that had none leaves it without changes to save.
  onRequest(undoCommand,
            [this](const muse::rcommand::Params &)
            {
              const auto master = projectSession()->masterNotation();
              const auto stack =
                  master ? master->notation()->undoStack() : nullptr;
              if (!stack || !stack->canUndo())
                return muse::make_ret(muse::Ret::Code::UnknownError,
                                      "nothing to undo");
              stack->undo(nullptr);
              return muse::make_ok();
            });
  onRequest(midiNoteCommand,
            [this](const muse::rcommand::Params &params)
            {
              const auto pitch = params.find("pitch");
              const auto velocity = params.find("velocity");
              if (pitch == params.end())
                return muse::make_ret(muse::Ret::Code::UnknownError,
                                      "no pitch");
              virtualMidiKeyboard()->note(
                  pitch->second.toInt(),
                  velocity == params.end() ? 0.0f : velocity->second.toFloat());
              return virtualKeyboardInUseRet(virtualMidiKeyboard()->inUse());
            });
  onRequest(midiPedalCommand,
            [this](const muse::rcommand::Params &params)
            {
              const auto it = params.find("down");
              virtualMidiKeyboard()->sustainPedal(it == params.end() ||
                                                  it->second.toBool());
              return virtualKeyboardInUseRet(virtualMidiKeyboard()->inUse());
            });
  onRequest(midiDeviceCommand,
            [this](const muse::rcommand::Params &params)
            {
              if (const QString id = stringParam(params, "id"); !id.isEmpty())
              {
                midiDeviceService()->selectDevice(ExternalDeviceId{
                    id == u"virtual" ? virtualMidiKeyboard()->deviceId()
                                     : id.toStdString()});
                if (id == u"virtual")
                  tutorials()->showVirtualKeyboard();
              }
              muse::Ret ret = muse::make_ok();
              const auto selected = midiDeviceService()->selectedDevice();
              ret.setText(selected ? selected->value : std::string{});
              return ret;
            });
  onRequestAsync(waitUntilStoppedCommand,
                 [this](const muse::rcommand::Params &, Done done)
                 { waitUntilStopped(std::move(done)); });
  onRequest(demoHideCommand,
            [this](const muse::rcommand::Params &)
            {
              m_demoCursor->hide();
              return muse::make_ok();
            });
  // As the window's close button does: OrchestrionActionController offers to
  // save the score's changes, and the user may cancel.
  onRequest(quitCommand,
            [this](const muse::rcommand::Params &)
            {
              mainWindow()->qWindow()->close();
              return muse::make_ok();
            });
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
  onRequestAsync(command, [handler](const muse::rcommand::Params &params,
                                    Done done) { done(handler(params)); });
}

void OrchestrionCommands::onRequestAsync(const muse::rcommand::Command &command,
                                         const AsyncHandler &handler)
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
                  handler(request.params,
                          [request, onResponse](const muse::Ret &ret) {
                            onResponse(
                                muse::rcommand::make_response(request, ret));
                          });
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

OrchestrionCommands::OrchestrionCommands(std::shared_ptr<DemoCursor> demoCursor,
                                         std::shared_ptr<DemoKeys> demoKeys)
    : m_demoCursor{std::move(demoCursor)}, m_demoKeys{std::move(demoKeys)}
{
}

void OrchestrionCommands::waitUntilStopped(Done done)
{
  // Polled, rather than subscribed to: the player is replaced with the
  // sequencer, when the score changes.
  const auto timer = std::make_shared<QTimer>();
  const auto check = [this, timer, done]
  {
    const auto player = orchestrion()->player();
    if (player && player->IsPlaying())
      return;
    timer->stop();
    done(muse::make_ok());
  };
  QObject::connect(timer.get(), &QTimer::timeout, check);
  timer->start(100);
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
