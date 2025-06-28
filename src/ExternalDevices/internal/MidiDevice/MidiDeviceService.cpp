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
#include "MidiDeviceService.h"

namespace dgk
{
using namespace ExternalDevicesUtils;

namespace
{
const ExternalDeviceId noDevice{muse::midi::NONE_DEVICE_ID};
}

void MidiDeviceService::init()
{
  midiInPort()->availableDevicesChanged().onNotify(
      this,
      [this]
      {
        const auto configDevice = configuration()->readSelectedMidiDevice();
        if (!configDevice)
        {
          const auto selected = selectedDeviceWithoutNoDevice();
          const auto available = availableDevicesWithoutNoDevice();
          if (!selected)
          {
            ScopedTrue scope{m_deviceChangeExpected};
            doSelectDevice(available.empty() ? noDevice : available.front());
          }
          else if (selected == noDevice && !available.empty())
          {
            ScopedTrue scope{m_deviceChangeExpected};
            doSelectDevice(available.front());
          }
          return;
        }

        const auto available = isAvailable(*configDevice);
        if (available && selectedDevice() == configDevice)
          return;

        ScopedTrue scope{m_deviceChangeExpected};
        doSelectDevice(available ? *configDevice : noDevice);
      });

  midiInPort()->deviceChanged().onNotify(
      this,
      [this]
      {
        if (!m_postInitCalled)
        {
          // We haven't done the post-init automatic selection yet, still, for
          // reliability, we should notify the change.
          m_selectedDeviceChanged.notify();
          return;
        }

        if (m_deviceChangeExpected)
          m_selectedDeviceChanged.notify();
        else if (const auto configDevice =
                     configuration()->readSelectedMidiDevice())
        {
          ScopedTrue scope{m_deviceChangeExpected};
          // MuseScore's midimodule reads from its own configuration on startup.
          // Catch this call and reset the device to our configuration.
          doSelectDevice(*configDevice);
        }
        else
          m_selectedDeviceChanged.notify();
      });
}

void MidiDeviceService::onAllInited()
{
  if (const auto configDevice = configuration()->readSelectedMidiDevice())
  {
    ScopedTrue scope{m_deviceChangeExpected};
    doSelectDevice(*configDevice);
  }
  else
  {
    const auto available = availableDevicesWithoutNoDevice();
    doSelectDevice(available.empty() ? noDevice : available.front());
  }

  // We don't want this post-init, start-up selection to trigger configuration
  // writing. Since device selection implementation for MIDI is synchronous, we
  // can do this.
  m_postInitCalled = true;
  m_startupSelectionFinished.notify();
}

muse::async::Notification MidiDeviceService::startupSelectionFinished() const
{
  return m_startupSelectionFinished;
}

std::vector<ExternalDeviceId> MidiDeviceService::availableDevices() const
{
  const muse::midi::MidiDeviceList midiDevices =
      midiInPort()->availableDevices();
  std::vector<ExternalDeviceId> ids;
  ids.reserve(midiDevices.size());
  for (const auto &device : midiDevices)
    ids.emplace_back(device.id);
  return ids;
}

std::vector<ExternalDeviceId>
MidiDeviceService::availableDevicesWithoutNoDevice() const
{
  auto devices = availableDevices();
  devices.erase(std::remove(devices.begin(), devices.end(), noDevice),
                devices.end());
  return devices;
}

bool MidiDeviceService::isAvailable(const ExternalDeviceId &id) const
{
  const muse::midi::MidiDeviceList midiDevices =
      midiInPort()->availableDevices();
  return std::any_of(midiDevices.begin(), midiDevices.end(),
                     [&id](const auto &device)
                     { return device.id == id.value; });
}

bool MidiDeviceService::isNoDevice(const ExternalDeviceId &id) const
{
  return id == noDevice;
}

muse::async::Notification MidiDeviceService::availableDevicesChanged() const
{
  return midiInPort()->availableDevicesChanged();
}

std::optional<ExternalDeviceId> MidiDeviceService::selectedDevice() const
{
  if (!midiInPort()->isConnected())
    return {};

  const auto id = midiInPort()->deviceID();
  if (id.empty())
    return {};

  return ExternalDeviceId{id};
}

std::optional<ExternalDeviceId>
MidiDeviceService::selectedDeviceWithoutNoDevice() const
{
  const auto id = selectedDevice();
  if (id && *id != noDevice)
    return id;
  return {};
}

void MidiDeviceService::selectDefaultDevice() { selectDevice(noDevice); }

void MidiDeviceService::selectDevice(const std::optional<ExternalDeviceId> &id)
{
  configuration()->writeSelectedMidiDevice(id);
  ScopedTrue scope{m_deviceChangeExpected};
  doSelectDevice(id.value_or(ExternalDeviceId{""}));
}

void MidiDeviceService::doSelectDevice(const ExternalDeviceId &id)
{
  if (id == selectedDevice())
    return;
  if (id.value.empty())
    midiInPort()->disconnect();
  else
    midiInPort()->connect(id.value);
}

muse::async::Notification MidiDeviceService::selectedDeviceChanged() const
{
  return m_selectedDeviceChanged;
}

std::string MidiDeviceService::deviceName(const ExternalDeviceId &id) const
{
  const muse::midi::MidiDeviceList midiDevices =
      midiInPort()->availableDevices();
  const auto it =
      std::find_if(midiDevices.begin(), midiDevices.end(),
                   [&id](const auto &device) { return device.id == id.value; });
  if (it != midiDevices.end())
    return it->name;
  return {};
}
} // namespace dgk