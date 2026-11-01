/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "VirtualMidiInPort.h"

#include <translation.h>

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
// Not of the form MuseScore gives its devices ("<index>:<client>:<port>"),
// so that it can't be taken for one of them.
const muse::midi::MidiDeviceID virtualDeviceId = "orchestrion:virtual-keyboard";

constexpr unsigned char noteOnStatus = 0x90;
constexpr unsigned char noteOffStatus = 0x80;
constexpr unsigned char controlChangeStatus = 0xB0;
constexpr unsigned char sustainPedalController = 64;
} // namespace

VirtualMidiInPort::VirtualMidiInPort(
    std::shared_ptr<muse::midi::IMidiInPort> port)
    : m_port{std::move(port)}
{
  m_port->deviceChanged().onNotify(this,
                                   [this]
                                   {
                                     // Another device connected: not this one
                                     if (m_port->isConnected())
                                       setInUse(false);
                                     m_deviceChanged.notify();
                                   });
  m_port->eventReceived().onReceive(
      this,
      [this](muse::midi::tick_t tick, const muse::midi::Event &event)
      {
        if (!m_inUse)
          m_eventReceived.send(tick, event);
      });
}

muse::midi::MidiDeviceList VirtualMidiInPort::availableDevices() const
{
  muse::midi::MidiDeviceList devices = m_port->availableDevices();
  // First: MidiDeviceService falls back on the last device listed
  devices.insert(
      devices.begin(),
      {virtualDeviceId, muse::trc("orchestrion", "Virtual keyboard")});
  return devices;
}

muse::async::Notification VirtualMidiInPort::availableDevicesChanged() const
{
  // Never unplugged: only the others come and go
  return m_port->availableDevicesChanged();
}

muse::Ret VirtualMidiInPort::connect(const muse::midi::MidiDeviceID &deviceID)
{
  if (deviceID != virtualDeviceId)
  {
    setInUse(false);
    return m_port->connect(deviceID);
  }
  if (m_inUse)
    return muse::make_ok();
  if (m_port->isConnected())
    m_port->disconnect();
  setInUse(true);
  m_deviceChanged.notify();
  return muse::make_ok();
}

void VirtualMidiInPort::disconnect()
{
  if (m_inUse)
  {
    setInUse(false);
    m_deviceChanged.notify();
    return;
  }
  m_port->disconnect();
}

bool VirtualMidiInPort::isConnected() const
{
  return m_inUse || m_port->isConnected();
}

muse::midi::MidiDeviceID VirtualMidiInPort::deviceID() const
{
  return m_inUse ? virtualDeviceId : m_port->deviceID();
}

muse::async::Notification VirtualMidiInPort::deviceChanged() const
{
  return m_deviceChanged;
}

muse::async::Channel<muse::midi::tick_t, muse::midi::Event>
VirtualMidiInPort::eventReceived() const
{
  return m_eventReceived;
}

std::string VirtualMidiInPort::deviceId() const { return virtualDeviceId; }

bool VirtualMidiInPort::inUse() const { return m_inUse; }

muse::async::Notification VirtualMidiInPort::inUseChanged() const
{
  return m_inUseChanged;
}

void VirtualMidiInPort::note(int pitch, float velocity)
{
  const auto key = static_cast<unsigned char>(std::clamp(pitch, 0, 127));
  // MIDI's 1 to 127: a struck key is never silent
  const auto value = static_cast<unsigned char>(
      std::clamp<long>(std::lround(velocity * 127), 0, 127));
  if (value == 0)
    send(noteOffStatus, key, 0);
  else
    send(noteOnStatus, key, value);
}

void VirtualMidiInPort::sustainPedal(bool down)
{
  send(controlChangeStatus, sustainPedalController, down ? 127 : 0);
}

void VirtualMidiInPort::setInUse(bool inUse)
{
  if (inUse == m_inUse)
    return;
  m_inUse = inUse;
  m_inUseChanged.notify();
}

void VirtualMidiInPort::send(unsigned char status, unsigned char data1,
                             unsigned char data2)
{
  if (!m_inUse)
    return;
  const unsigned char bytes[] = {status, data1, data2};
  m_eventReceived.send(0, muse::midi::Event::fromMidi10Bytes(bytes, 3));
}
} // namespace dgk
