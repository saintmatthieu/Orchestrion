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

#include "global/async/asyncable.h"
#include "global/async/notification.h"
#include "midi/imidiinport.h"
#include "global/modularity/ioc.h"

namespace dgk
{
class MidiDeviceService : public IMidiDeviceService,
                          public muse::Injectable,
                          public muse::async::Asyncable
{
public:
  void init();
  void onAllInited();

  muse::async::Notification startupSelectionFinished() const override;
  muse::async::Notification activityDetected() const override;

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
  std::vector<ExternalDeviceId> availableDevicesWithoutNoDevice() const;
  std::optional<ExternalDeviceId> selectedDeviceWithoutNoDevice() const;

  muse::Inject<muse::midi::IMidiInPort> midiInPort;
  muse::Inject<IExternalDevicesConfiguration> configuration;
  muse::async::Notification m_activityDetected;
  muse::async::Notification m_selectedDeviceChanged;
  muse::async::Notification m_startupSelectionFinished;
  bool m_deviceChangeExpected = false;
  bool m_postInitCalled = false;
};
} // namespace dgk