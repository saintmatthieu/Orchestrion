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
#include "OrchestrionCommon/OrchestrionCommonTypes.h"
#include "OrchestrionShellTypes.h"
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <async/channel.h>
#include <async/notification.h>
#include <global/settings.h>
#include <memory>
#include <modularity/ioc.h>

namespace dgk
{
class DeviceMenuManager : public muse::actions::Actionable,
                          public muse::async::Asyncable,
                          public muse::Injectable,
                          public std::enable_shared_from_this<DeviceMenuManager>
{
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher = {this};

public:
  DeviceMenuManager(DeviceType);
  virtual ~DeviceMenuManager() = default;

  void init();
  virtual std::string getMenuId(int deviceIndex) const = 0;
  virtual std::string selectedDevice() const = 0;
  std::vector<DeviceAction> settableDevices() const;
  muse::async::Notification settableDevicesChanged() const;
  muse::async::Channel<std::string> selectedPlaybackDeviceChanged() const;

protected:
  muse::Settings *settings();

private:
  struct MenuEntry : public DeviceDesc
  {
    MenuEntry(std::string id, std::string name, int index)
        : DeviceDesc{std::move(id), std::move(name)}, index{index}
    {
    }
    int index;
  };

  virtual muse::async::Notification availableDevicesChanged() const = 0;
  virtual std::vector<DeviceDesc> availableDevices() const = 0;
  virtual bool selectDevice(const std::string &deviceId) = 0;
  virtual void doInit() {}
  void fillDeviceCache();
  muse::Settings::Key defaultDeviceIdKey() const;

  const DeviceType m_deviceType;
  muse::async::Notification m_settableDevicesChanged;
  muse::async::Channel<std::string> m_selectedPlaybackDeviceChanged;
  std::vector<MenuEntry> m_deviceCache;
};
} // namespace dgk