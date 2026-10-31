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
#include "OrchestrionUiActions.h"
#include "DeviceMenuManager.h"
#include "MuseScoreShell/OrchestrionActionIds.h"
#include "shortcuts/shortcutcontext.h"
#include "ui/uiaction.h"

namespace dgk
{
namespace
{
muse::ui::UiActionList makeActions(
    const std::unordered_map<DeviceType, std::shared_ptr<DeviceMenuManager>>
        &managers)
{
  muse::ui::UiActionList actions{
      muse::ui::UiAction(actionIds::browseScores, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-file-open", muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-search-musescore", muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-file-help", muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-file-save", muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-file-save-as", muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::clearRecentFiles, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::revertToFactorySettings, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-recording",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-note-info",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::loopIn, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::loopOut, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-tempo-visualization",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::toggleJumpAnticipation, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-grading",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-persistent-timing-marks",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-hand-sync-score",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-dynamics-score",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::toggleGradingExposure, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::toggleTakeRecording, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::replayTake, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("orchestrion-advanced-toggle-proportional-spacing",
                         muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::playbackToggle, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::playbackStop, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::gradingSettings, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::playModePerformance, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::playModeFittedTempo, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::playModeMetronome, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::ornamentsDisabled, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::ornamentsManual, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::ornamentsAutomatic, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::addEffect, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::removeEffect, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::editEffect, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("view-toggle-fullscreen", muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::toggleMidiKeyboardIcon, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::togglePedalIndicator, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::toggleBarNumbers, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::setGoldTheme, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction(actionIds::setSilverTheme, muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("prev", muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("next", muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY),
      muse::ui::UiAction("rewind", muse::ui::UiCtxAny,
                         muse::shortcuts::CTX_ANY)};

  for (const auto &[_, menuId] : actionIds::chooseDevicesSubmenu)
    actions.push_back(muse::ui::UiAction(menuId, muse::ui::UiCtxAny,
                                         muse::shortcuts::CTX_ANY));

  // reserve some actions for playback device selection
  actions.reserve(100 + actions.size());
  for (auto i = 0; i < 100; ++i)
    for (const auto &[_, manager] : managers)
      actions.push_back(muse::ui::UiAction(
          manager->getMenuId(i), muse::ui::UiCtxAny, muse::shortcuts::CTX_ANY));

  return actions;
}
} // namespace

OrchestrionUiActions::OrchestrionUiActions(
    std::shared_ptr<DeviceMenuManager> midiControllerMenuManager,
    std::shared_ptr<DeviceMenuManager> midiSynthesizerMenuManager,
    std::shared_ptr<DeviceMenuManager> playbackDeviceMenuManager)
    : m_deviceMenuManagers{{DeviceType::MidiController,
                            midiControllerMenuManager},
                           {DeviceType::MidiSynthesizer,
                            midiSynthesizerMenuManager},
                           {DeviceType::PlaybackDevice,
                            playbackDeviceMenuManager}},
      m_actions{makeActions(m_deviceMenuManagers)}
{
}

void OrchestrionUiActions::init()
{
  m_deviceMenuManagers.at(DeviceType::MidiController)->init();
  m_deviceMenuManagers.at(DeviceType::MidiSynthesizer)->init();
  m_deviceMenuManagers.at(DeviceType::PlaybackDevice)->init();
}

muse::async::Notification
OrchestrionUiActions::settableDevicesChanged(DeviceType deviceType) const
{
  return m_deviceMenuManagers.at(deviceType)->settableDevicesChanged();
}

std::vector<DeviceAction>
OrchestrionUiActions::settableDevices(DeviceType deviceType) const
{
  return m_deviceMenuManagers.at(deviceType)->settableDevices();
}

std::string OrchestrionUiActions::selectedDevice(DeviceType deviceType) const
{
  return m_deviceMenuManagers.at(deviceType)->selectedDevice();
}

muse::async::Channel<std::string>
OrchestrionUiActions::selectedDeviceChanged(DeviceType deviceType) const
{
  return m_deviceMenuManagers.at(deviceType)->selectedPlaybackDeviceChanged();
}

const muse::ui::UiActionList &OrchestrionUiActions::actionsList() const
{
  return m_actions;
}

bool OrchestrionUiActions::actionEnabled(const muse::ui::UiAction &) const
{
  return true;
}

muse::async::Channel<muse::actions::ActionCodeList>
OrchestrionUiActions::actionEnabledChanged() const
{
  return m_actionEnabledChanged;
}

bool OrchestrionUiActions::actionChecked(const muse::ui::UiAction &) const
{
  return false;
}

muse::async::Channel<muse::actions::ActionCodeList>
OrchestrionUiActions::actionCheckedChanged() const
{
  return m_actionCheckedChanged;
}
} // namespace dgk
