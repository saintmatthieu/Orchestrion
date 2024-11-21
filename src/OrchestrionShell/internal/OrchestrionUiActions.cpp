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
#include "MuseScoreShell/OrchestrionActionIds.h"
#include "context/shortcutcontext.h"
#include "context/uicontext.h"

namespace dgk::orchestrion
{
namespace
{
std::string menuIdFromDeviceNameIndex(int index)
{
  return "choosePlaybackDevice_" + std::to_string(index);
}

muse::ui::UiActionList makeActions()
{
  muse::ui::UiActionList actions;
  actions.push_back(muse::ui::UiAction(actionIds::choosePlaybackDeviceSubmenu,
                                       mu::context::UiCtxAny,
                                       mu::context::CTX_ANY));

  // reserve some actions for playback device selection
  actions.reserve(100 + actions.size());
  for (auto i = 0; i < 100; ++i)
    actions.push_back(muse::ui::UiAction(menuIdFromDeviceNameIndex(i),
                                         mu::context::UiCtxAny,
                                         mu::context::CTX_ANY));
  return actions;
}
} // namespace

OrchestrionUiActions::OrchestrionUiActions() : m_actions{makeActions()} {}

void OrchestrionUiActions::init()
{
  audioDriver()->availableOutputDevicesChanged().onNotify(
      this,
      [this]
      {
        fillDeviceCache();
        m_settablePlaybackDevicesChanged.notify();
      });
  fillDeviceCache();
}

void OrchestrionUiActions::fillDeviceCache()
{
  std::for_each(m_deviceCache.begin(), m_deviceCache.end(),
                [this](DeviceItem &device) { device.available = false; });
  for (const auto &device : audioDriver()->availableOutputDevices())
  {
    const auto it = std::find_if(m_deviceCache.begin(), m_deviceCache.end(),
                                 [&device](const DeviceItem &item)
                                 { return item.id == device.id; });
    if (it == m_deviceCache.end())
    {
      // New device
      const auto menuId = menuIdFromDeviceNameIndex(m_deviceCache.size());
      dispatcher()->reg(this, menuId, [this, device]()
                        { audioDriver()->selectOutputDevice(device.id); });
      m_deviceCache.push_back({device.id, device.name, true});
    }
    else
      // Device already known
      it->available = true;
  }
}

muse::async::Notification
OrchestrionUiActions::settablePlaybackDevicesChanged() const
{
  return m_settablePlaybackDevicesChanged;
}

std::vector<DeviceAction> OrchestrionUiActions::settablePlaybackDevices() const
{
  std::vector<DeviceAction> devices;
  auto i = 0;
  std::for_each(m_deviceCache.begin(), m_deviceCache.end(),
                [&](const DeviceItem &device)
                {
                  if (device.available)
                    devices.push_back(
                        {menuIdFromDeviceNameIndex(i), device.name});
                  ++i;
                });
  return devices;
}

const muse::ui::UiActionList &OrchestrionUiActions::actionsList() const
{
  return m_actions;
}

bool OrchestrionUiActions::actionEnabled(const muse::ui::UiAction &act) const
{
  return true;
}

muse::async::Channel<muse::actions::ActionCodeList>
OrchestrionUiActions::actionEnabledChanged() const
{
  return m_actionEnabledChanged;
}

bool OrchestrionUiActions::actionChecked(const muse::ui::UiAction &act) const
{
  return false;
}

muse::async::Channel<muse::actions::ActionCodeList>
OrchestrionUiActions::actionCheckedChanged() const
{
  return m_actionCheckedChanged;
}
} // namespace dgk::orchestrion
