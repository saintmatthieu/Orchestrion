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

#include "DeviceMenuManager.h"
#include "IPlaybackDeviceManager.h"
#include <async/asyncable.h>
#include <audio/iaudiodriver.h>

namespace dgk
{
class PlaybackDeviceMenuManager : public IPlaybackDeviceManager,
                                  public DeviceMenuManager
{
public:
  PlaybackDeviceMenuManager();

private:
  muse::Inject<muse::audio::IAudioDriver> audioDriver = {this};

  // DeviceMenuManager
private:
  muse::async::Notification availableDevicesChanged() const override;
  std::vector<DeviceDesc> availableDevices() const override;
  std::string getMenuId(int deviceIndex) const override;
  std::string selectedDevice() const override;
  bool selectDevice(const std::string &deviceId) override;
  void doInit() override;

  // IPlaybackDeviceManager
private:
  void trySelectDefaultDevice() override;

private:
  void updateAudioDevices();

  muse::async::Notification m_availableDevicesChanged;
  muse::audio::AudioDeviceList m_audioDevices;
};
} // namespace dgk
