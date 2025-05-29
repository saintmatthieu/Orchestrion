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

namespace dgk
{
PlaybackDeviceMenuManager::PlaybackDeviceMenuManager()
    : DeviceMenuManager(DeviceType::PlaybackDevice)
{
}

muse::async::Notification
PlaybackDeviceMenuManager::availableDevicesChanged() const
{
  return audioDeviceService()->availableDevicesChanged();
}

std::vector<DeviceDesc> PlaybackDeviceMenuManager::availableDevices() const
{
  const std::vector<ExternalDeviceId> ids =
      audioDeviceService()->availableDevices();
  std::vector<DeviceDesc> descriptions;
  descriptions.reserve(ids.size());
  std::transform(
      ids.begin(), ids.end(), std::back_inserter(descriptions),
      [this](const ExternalDeviceId &id)
      { return DeviceDesc{id.value, audioDeviceService()->deviceName(id)}; });
  return descriptions;
}

std::string PlaybackDeviceMenuManager::getMenuId(int deviceIndex) const
{
  return "chooseAudioDevice_" + std::to_string(deviceIndex);
}

std::string PlaybackDeviceMenuManager::selectedDevice() const
{
  return audioDeviceService()
      ->selectedDevice()
      .value_or(ExternalDeviceId{""})
      .value;
}

bool PlaybackDeviceMenuManager::selectDevice(const std::string &deviceId)
{
  const ExternalDeviceId id{deviceId};
  if (!audioDeviceService()->isAvailable(id))
    return false;
  audioDeviceService()->selectDevice(id);
  return true;
}
} // namespace dgk