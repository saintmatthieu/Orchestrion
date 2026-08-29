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

#include "../ExternalDevicesUtils.h"
#include "IExternalDevicesConfiguration.h"
#include "IMidiDeviceService.h"

#include "async/asyncable.h"
#include "async/notification.h"
#include "midi/imidiinport.h"
#include "modularity/ioc.h"

namespace dgk
{
//! Which MIDI controller feeds the app. Greedy by default: whatever is
//! plugged in gets used, and a newly plugged-in device takes over from the
//! current one; when the device in use goes away, the most recently
//! enumerated remaining one steps in. A device expressly chosen in the
//! Audio/MIDI menu (persisted) overrides this whenever it is available —
//! "no device" always is, so choosing it silences MIDI input for good.
class MidiDeviceService : public IMidiDeviceService,
                          public muse::Injectable,
                          public muse::async::Asyncable
{
public:
  void init();
  void onAllInited();

  std::vector<ExternalDeviceId> availableDevices() const override;
  muse::async::Notification availableDevicesChanged() const override;
  bool isAvailable(const ExternalDeviceId &) const override;
  bool isNoDevice(const ExternalDeviceId &id) const override;

  void selectDevice(const std::optional<ExternalDeviceId> &) override;
  muse::async::Notification selectedDeviceChanged() const override;
  std::optional<ExternalDeviceId> selectedDevice() const override;

  void selectDefaultDevice() override;

  std::string deviceName(const ExternalDeviceId &) const override;

private:
  void doSelectDevice(const ExternalDeviceId &);
  void onAvailableDevicesChanged();
  //! The device to use now. \p newcomer is a device that has just appeared;
  //! \p keepCurrent whether the device presently connected has a say (it
  //! doesn't when it is MuseScore's own startup choice, not ours).
  ExternalDeviceId
  preferredDevice(const std::optional<ExternalDeviceId> &newcomer,
                  bool keepCurrent) const;
  std::vector<ExternalDeviceId> availableDevicesWithoutNoDevice() const;
  std::optional<ExternalDeviceId> selectedDeviceWithoutNoDevice() const;

  muse::Inject<muse::midi::IMidiInPort> midiInPort;
  muse::Inject<IExternalDevicesConfiguration> configuration;
  muse::async::Notification m_selectedDeviceChanged;
  bool m_deviceChangeExpected = false;
  bool m_postInitCalled = false;
  //! The devices seen at the last check, to tell newcomers apart.
  std::vector<ExternalDeviceId> m_knownDevices;
};
} // namespace dgk