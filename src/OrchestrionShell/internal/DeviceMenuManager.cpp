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

namespace dgk
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
  case DeviceType::MidiSynthesizer:
    return muse::Settings::Key{"midi", "default MIDI synthesizer ID"};
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

muse::Settings *DeviceMenuManager::settings()
{
  return muse::Settings::instance();
}

std::vector<DeviceAction> DeviceMenuManager::settableDevices() const
{
  const std::vector<DeviceDesc> descs = availableDevices();
  std::vector<DeviceAction> actions;
  actions.reserve(descs.size());
  auto i = 0;
  std::for_each(m_deviceCache.begin(), m_deviceCache.end(),
                [&](const MenuEntry &entry)
                {
                  if (std::find_if(descs.begin(), descs.end(),
                                   [&entry](const DeviceDesc &d)
                                   { return d.id == entry.id; }) != descs.end())
                    actions.emplace_back(getMenuId(entry.index), entry.name, entry.id);
                  ++i;
                });
  return actions;
}

muse::async::Notification DeviceMenuManager::settableDevicesChanged() const
{
  return m_settableDevicesChanged;
}

muse::async::Channel<std::string>
DeviceMenuManager::selectedPlaybackDeviceChanged() const
{
  return m_selectedPlaybackDeviceChanged;
}

void DeviceMenuManager::fillDeviceCache()
{
  for (const auto &device : availableDevices())
  {
    const auto it = std::find_if(m_deviceCache.begin(), m_deviceCache.end(),
                                 [&device](const DeviceDesc &item)
                                 { return item.id == device.id; });
    if (it == m_deviceCache.end())
    {
      const auto index = static_cast<int>(m_deviceCache.size());
      const auto menuId = getMenuId(index);
      dispatcher()->reg(this, menuId,
                        [this, wt = weak_from_this(), deviceId = device.id]()
                        {
                          const auto lifeguard = wt.lock();
                          if (!lifeguard)
                            return;
                          if (selectDevice(deviceId))
                            m_selectedPlaybackDeviceChanged.send(deviceId);
                        });

      // The no-device entry first.
      if (device.id == "-1")
        m_deviceCache.emplace(m_deviceCache.begin(), device.id, device.name, index);
      else
        m_deviceCache.emplace_back(device.id, device.name, index);
    }
  }
}
} // namespace dgk