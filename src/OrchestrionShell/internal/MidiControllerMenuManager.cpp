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
#include "MidiControllerMenuManager.h"

namespace dgk::orchestrion
{
muse::async::Notification
MidiControllerMenuManager::availableDevicesChanged() const
{
  return midiInPort()->availableDevicesChanged();
}

std::vector<DeviceMenuManager::DeviceDesc>
MidiControllerMenuManager::availableDevices() const
{
  const auto midiDevices = midiInPort()->availableDevices();
  std::vector<DeviceDesc> descriptions(midiDevices.size());
  std::transform(midiDevices.begin(), midiDevices.end(), descriptions.begin(),
                 [](const auto &device)
                 { return DeviceDesc{device.id, device.name}; });
  return descriptions;
}

std::string MidiControllerMenuManager::getMenuId(int deviceIndex) const
{
  return "chooseMidiDevice_" + std::to_string(deviceIndex);
}

std::string MidiControllerMenuManager::selectedDevice() const
{
  return midiInPort()->deviceID();
}

bool MidiControllerMenuManager::selectDevice(const std::string &deviceId)
{
  return midiInPort()->connect(deviceId).success();
}
} // namespace dgk::orchestrion