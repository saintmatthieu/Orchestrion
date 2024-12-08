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
#include "DeviceMenuManager.h"
#include <algorithm>

namespace dgk::orchestrion
{
DeviceMenuManager::DeviceMenuManager(DeviceType deviceType)
    : m_deviceType(deviceType)
{
}

muse::Settings::Key DeviceMenuManager::defaultDeviceIdKey() const
{
  switch (m_deviceType)
  {
  case DeviceType::MidiController:
    return muse::Settings::Key{"midi", "default MIDI controller ID"};
  case DeviceType::PlaybackDevice:
    return muse::Settings::Key{"audio", "default playback device ID"};
  }
  assert(false);
  return {};
}

void DeviceMenuManager::init()
{
  doInit();
  availableDevicesChanged().onNotify(this,
                                     [this]
                                     {
                                       fillDeviceCache();
                                       m_settableDevicesChanged.notify();
                                     });
  fillDeviceCache();
}

void DeviceMenuManager::doTrySelectDefaultDevice()
{
  if (const auto value = settings()->value(defaultDeviceIdKey());
      !value.isNull())
  {
    const auto deviceId = value.toString();
    if (!deviceId.empty())
      selectDevice(deviceId);
  }
}

muse::Settings *DeviceMenuManager::settings()
{
  return muse::Settings::instance();
}

std::vector<DeviceAction> DeviceMenuManager::settableDevices() const
{
  std::vector<DeviceAction> devices;
  auto i = 0;
  std::for_each(m_deviceCache.begin(), m_deviceCache.end(),
                [&](const DeviceItem &device)
                {
                  if (device.available)
                    devices.emplace_back(getMenuId(i), device.name, device.id);
                  ++i;
                });
  return devices;
}

muse::async::Notification DeviceMenuManager::settableDevicesChanged() const
{
  return m_settableDevicesChanged;
}

std::string DeviceMenuManager::lastSelectedDevice() const
{
  return m_lastSelectedDevice;
}

void DeviceMenuManager::onDeviceSuccessfullySet(const std::string &deviceId)
{
  assert(!deviceId.empty());
  if (deviceId.empty())
    return;
  m_lastSelectedDevice = deviceId;
  settings()->setSharedValue(defaultDeviceIdKey(), muse::Val{deviceId});
}

muse::async::Channel<std::string>
DeviceMenuManager::selectedPlaybackDeviceChanged() const
{
  return m_selectedPlaybackDeviceChanged;
}

void DeviceMenuManager::fillDeviceCache()
{
  std::for_each(m_deviceCache.begin(), m_deviceCache.end(),
                [this](DeviceItem &device) { device.available = false; });
  for (const auto &device : availableDevices())
  {
    const auto it = std::find_if(m_deviceCache.begin(), m_deviceCache.end(),
                                 [&device](const DeviceItem &item)
                                 { return item.id == device.id; });
    if (it == m_deviceCache.end())
    {
      // New device
      const auto menuId = getMenuId(m_deviceCache.size());
      dispatcher()->reg(this, menuId,
                        [this, wt = weak_from_this(), deviceId = device.id]()
                        {
                          const auto lifeguard = wt.lock();
                          if (!lifeguard)
                            return;
                          if (selectDevice(deviceId))
                            m_selectedPlaybackDeviceChanged.send(deviceId);
                        });
      m_deviceCache.push_back({device.id, device.name, true});
    }
    else
      // Device already known
      it->available = true;
  }
}

} // namespace dgk::orchestrion