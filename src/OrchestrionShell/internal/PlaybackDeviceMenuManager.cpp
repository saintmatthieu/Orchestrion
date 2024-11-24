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
#include "PlaybackDeviceMenuManager.h"
#include <future>

namespace dgk::orchestrion
{
void PlaybackDeviceMenuManager::doInit()
{
  audioDriver()->availableOutputDevicesChanged().onNotify(
      this, [this] { updateAudioDevices(); });
  updateAudioDevices();
}

void PlaybackDeviceMenuManager::updateAudioDevices()
{
  std::promise<muse::audio::AudioDeviceList> promise;
  auto future = promise.get_future();
  std::thread([&]
              { promise.set_value(audioDriver()->availableOutputDevices()); })
      .detach();
  m_audioDevices = future.get();
  m_availableDevicesChanged.notify();
}

muse::async::Notification
PlaybackDeviceMenuManager::availableDevicesChanged() const
{
  return m_availableDevicesChanged;
}

std::vector<DeviceMenuManager::DeviceDesc>
PlaybackDeviceMenuManager::availableDevices() const
{
  std::vector<DeviceDesc> descriptions(m_audioDevices.size());
  std::transform(m_audioDevices.begin(), m_audioDevices.end(),
                 descriptions.begin(), [](const auto &device)
                 { return DeviceDesc{device.id, device.name}; });
  return descriptions;
}

std::string PlaybackDeviceMenuManager::getMenuId(int deviceIndex) const
{
  return "chooseAudioDevice_" + std::to_string(deviceIndex);
}

std::string PlaybackDeviceMenuManager::selectedDevice() const
{
  return audioDriver()->outputDevice();
}

bool PlaybackDeviceMenuManager::selectDevice(const std::string &deviceId)
{
  return audioDriver()->selectOutputDevice(deviceId);
}
} // namespace dgk::orchestrion