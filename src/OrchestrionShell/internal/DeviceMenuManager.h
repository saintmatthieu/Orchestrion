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

#include "OrchestrionShellTypes.h"
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "async/channel.h"
#include "async/notification.h"
#include "modularity/ioc.h"
#include <memory>

namespace dgk::orchestrion
{
class DeviceMenuManager : public muse::actions::Actionable,
                          public muse::async::Asyncable,
                          public muse::Injectable,
                          public std::enable_shared_from_this<DeviceMenuManager>
{
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher = {this};

public:
  virtual ~DeviceMenuManager() = default;

  void init();
  virtual std::string getMenuId(int deviceIndex) const = 0;
  std::vector<DeviceAction> settableDevices() const;
  muse::async::Notification settableDevicesChanged() const;
  virtual std::string selectedDevice() const = 0;
  muse::async::Channel<std::string> selectedPlaybackDeviceChanged() const;

protected:
  struct DeviceDesc
  {
    std::string id;
    std::string name;
  };

private:
  struct DeviceItem : DeviceDesc
  {
    bool available = true;
  };

  virtual muse::async::Notification availableDevicesChanged() const = 0;
  virtual std::vector<DeviceDesc> availableDevices() const = 0;
  virtual bool selectDevice(const std::string &deviceId) = 0;
  virtual void doInit() {}
  void fillDeviceCache();

  muse::async::Notification m_settableDevicesChanged;
  muse::async::Channel<std::string> m_selectedPlaybackDeviceChanged;
  std::vector<DeviceItem> m_deviceCache;
};
} // namespace dgk::orchestrion