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

#include "DeviceMenuManager.h"

#include "ExternalDevices/IMidiDeviceService.h"
#include <global/iinteractive.h>

namespace dgk
{
class ControllerMenuManager : public DeviceMenuManager
{
public:
  ControllerMenuManager();

private:
  muse::Inject<IMidiDeviceService> midiDeviceService;
  muse::Inject<muse::IInteractive> interactive;

  // DeviceMenuManager
private:
  std::string selectedDevice() const override;
  muse::async::Notification availableDevicesChanged() const override;
  std::vector<DeviceDesc> availableDevices() const override;
  std::string getMenuId(int deviceIndex) const override;
  bool selectDevice(const std::string &deviceId) override;

private:
  bool maybePromptUser(const std::string &deviceId);};
} // namespace dgk
