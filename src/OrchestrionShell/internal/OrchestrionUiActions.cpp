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
#include "context/shortcutcontext.h"
#include "context/uicontext.h"

namespace dgk
{
namespace
{
muse::ui::UiActionList makeActions(
    const std::unordered_map<DeviceType, std::shared_ptr<DeviceMenuManager>>
        &managers)
{
  muse::ui::UiActionList actions{
      muse::ui::UiAction("orchestrion-file-open", mu::context::UiCtxAny,
                         mu::context::CTX_ANY),
      muse::ui::UiAction("orchestrion-file-close",
                         mu::context::UiCtxNotationOpened,
                         mu::context::CTX_NOTATION_OPENED)};

  for (const auto &[_, menuId] : actionIds::chooseDevicesSubmenu)
    actions.push_back(muse::ui::UiAction(menuId, mu::context::UiCtxAny,
                                         mu::context::CTX_ANY));

  // reserve some actions for playback device selection
  actions.reserve(100 + actions.size());
  for (auto i = 0; i < 100; ++i)
    for (const auto &[_, manager] : managers)
      actions.push_back(muse::ui::UiAction(
          manager->getMenuId(i), mu::context::UiCtxAny, mu::context::CTX_ANY));

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
