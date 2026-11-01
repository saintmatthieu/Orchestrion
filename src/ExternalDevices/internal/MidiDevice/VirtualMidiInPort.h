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
#pragma once

#include "IVirtualMidiKeyboard.h"

#include <async/asyncable.h>
#include <midi/imidiinport.h>

#include <memory>

namespace dgk
{
/**
 * MuseScore's MIDI input port, with the virtual keyboard among its devices:
 * registered in its stead, so that everything that lists, connects or
 * listens to MIDI input devices (the Audio/MIDI menu, MidiDeviceService, the
 * gesture controllers, MuseScore's own note input) takes the virtual keyboard
 * for one more device. The real ones go through to MuseScore's port.
 */
class VirtualMidiInPort : public muse::midi::IMidiInPort,
                          public IVirtualMidiKeyboard,
                          public muse::async::Asyncable
{
public:
  explicit VirtualMidiInPort(std::shared_ptr<muse::midi::IMidiInPort> port);

  // IMidiInPort
  muse::midi::MidiDeviceList availableDevices() const override;
  muse::async::Notification availableDevicesChanged() const override;
  muse::Ret connect(const muse::midi::MidiDeviceID &deviceID) override;
  void disconnect() override;
  bool isConnected() const override;
  muse::midi::MidiDeviceID deviceID() const override;
  muse::async::Notification deviceChanged() const override;
  muse::async::Channel<muse::midi::tick_t, muse::midi::Event>
  eventReceived() const override;

  // IVirtualMidiKeyboard
  std::string deviceId() const override;
  bool inUse() const override;
  muse::async::Notification inUseChanged() const override;
  void note(int pitch, float velocity) override;
  void sustainPedal(bool down) override;

private:
  void setInUse(bool inUse);
  void send(unsigned char status, unsigned char data1, unsigned char data2);

  const std::shared_ptr<muse::midi::IMidiInPort> m_port;
  bool m_inUse = false;
  muse::async::Notification m_deviceChanged;
  muse::async::Notification m_inUseChanged;
  muse::async::Channel<muse::midi::tick_t, muse::midi::Event> m_eventReceived;
};
} // namespace dgk
