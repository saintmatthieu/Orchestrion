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
#include "log.h"

namespace dgk::orchestrion
{
void MidiControllerMenuManager::doInit()
{
  multiInstances()->otherInstanceGainedFocus().onNotify(
      this,
      [this]
      {
        if (midiInPort()->isConnected())
        {
          const auto masterNotation = globalContext()->currentMasterNotation();
          assert(masterNotation);
          LOGI() << "Releasing control of MIDI device for "
                 << (masterNotation ? masterNotation->notation()->name()
                                    : "null");
          midiInPort()->disconnect();
        }
      });
  m_selectedDevice = midiInPort()->deviceID();
}

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
  assert(m_selectedDevice == midiInPort()->deviceID());
  return midiInPort()->deviceID();
}

bool MidiControllerMenuManager::selectDevice(const std::string &deviceId)
{
  if (midiInPort()->connect(deviceId))
  {
    m_selectedDevice = deviceId;
    return true;
  }
  return false;
}

void MidiControllerMenuManager::onGainedFocus()
{
  if (midiInPort()->deviceID() == m_selectedDevice)
    return;
  LOGI() << "Reconnecting previously selected device: "
         << midiInPort()->connect(m_selectedDevice);
}
} // namespace dgk::orchestrion