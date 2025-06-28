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

#include "IAudioDeviceService.h"
#include "IExternalDevicesConfiguration.h"

#include "async/async.h"
#include "async/asyncable.h"
#include "async/notification.h"
#include "audio/iaudiodriver.h"
#include "modularity/ioc.h"

namespace dgk
{
class AudioDeviceService : public IAudioDeviceService,
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
  std::vector<muse::audio::AudioDevice> museAvailableDevices() const;
  void doSelectDevice(const ExternalDeviceId &id);
  muse::Inject<IExternalDevicesConfiguration> configuration;
  muse::Inject<muse::audio::IAudioDriver> audioDriver;
  muse::async::Notification m_selectedDeviceChanged;
  bool m_deviceChangeExpected = false;
  bool m_postInitCalled = false;
};
} // namespace dgk