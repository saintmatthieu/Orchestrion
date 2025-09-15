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

#include "MuseScoreShell/MuseScoreShellTypes.h"
#include "OrchestrionShellTypes.h"
#include "global/async/channel.h"
#include "global/async/notification.h"
#include "global/modularity/imoduleinterface.h"

namespace dgk
{
class IOrchestrionUiActions : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionUiActions);

public:
  virtual ~IOrchestrionUiActions() = default;

  virtual muse::async::Notification
      settableDevicesChanged(DeviceType) const = 0;
  virtual std::vector<DeviceAction> settableDevices(DeviceType) const = 0;
  virtual std::string selectedDevice(DeviceType) const = 0;
  virtual muse::async::Channel<std::string>
      selectedDeviceChanged(DeviceType) const = 0;
};
} // namespace dgk
