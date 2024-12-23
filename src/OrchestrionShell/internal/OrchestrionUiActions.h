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

#include "IOrchestrionUiActions.h"
#include "ui/iuiactionsmodule.h"
#include <unordered_map>

namespace dgk::orchestrion
{
class DeviceMenuManager;

class OrchestrionUiActions : public IOrchestrionUiActions,
                             public muse::ui::IUiActionsModule
{
public:
  OrchestrionUiActions(
      std::shared_ptr<DeviceMenuManager> midiControllerMenuManager,
      std::shared_ptr<DeviceMenuManager> midiSynthesizerMenuManager,
      std::shared_ptr<DeviceMenuManager> playbackDeviceMenuManager);

  void init();

  // IOrchestrionUiActions
private:
  muse::async::Notification settableDevicesChanged(DeviceType) const override;
  std::vector<DeviceAction> settableDevices(DeviceType) const override;
  std::string selectedDevice(DeviceType) const override;
  muse::async::Channel<std::string>
      selectedDeviceChanged(DeviceType) const override;

  // muse::ui::IUiActionsModule
private:
  const muse::ui::UiActionList &actionsList() const override;
  bool actionEnabled(const muse::ui::UiAction &act) const override;
  muse::async::Channel<muse::actions::ActionCodeList>
  actionEnabledChanged() const override;

  bool actionChecked(const muse::ui::UiAction &act) const override;
  muse::async::Channel<muse::actions::ActionCodeList>
  actionCheckedChanged() const override;

private:
  const std::unordered_map<DeviceType, std::shared_ptr<DeviceMenuManager>>
      m_deviceMenuManagers;
  const muse::ui::UiActionList m_actions;
  muse::async::Channel<muse::actions::ActionCodeList> m_actionEnabledChanged;
  muse::async::Channel<muse::actions::ActionCodeList> m_actionCheckedChanged;
};
} // namespace dgk::orchestrion
