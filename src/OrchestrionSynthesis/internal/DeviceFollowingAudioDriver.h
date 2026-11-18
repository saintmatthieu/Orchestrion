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

#include <async/asyncable.h>
#include <audio/iaudiodriver.h>
#include <audio/main/internal/iaudiodriverfactory.h>

#include <memory>

namespace dgk
{
/**
 * The platform's driver, following the output device, where the platform
 * tells of it (see OutputDevice.h):
 *
 * - it opens the device at the rate the device already runs at, whatever
 *   rate is asked for: a driver that sets the device's rate (macOS' direct
 *   CoreAudio driver) would otherwise change it for the whole system, and one
 *   that doesn't would resample. Orchestrion lets no rate be chosen;
 * - asked for the system's default device, it stays on it: it reopens on the
 *   new one when the system's default changes, and its active spec names the
 *   default rather than the device it resolved to (what is shown as chosen,
 *   and stored);
 * - it has the device reopened when its rate is changed by someone else.
 *
 * It doesn't reopen the device itself: it tells that the devices changed, and
 * AudioDriverController reopens it as when devices come and go, falling back
 * on the default device, then on the default driver, if it fails.
 */
class DeviceFollowingAudioDriver
    : public muse::audio::IAudioDriver,
      public muse::async::Asyncable,
      public std::enable_shared_from_this<DeviceFollowingAudioDriver>
{
public:
  explicit DeviceFollowingAudioDriver(muse::audio::IAudioDriverPtr driver);

  void init() override { m_driver->init(); }
  std::string name() const override { return m_driver->name(); }
  muse::audio::AudioDeviceID defaultDevice() const override
  {
    return m_driver->defaultDevice();
  }
  muse::audio::AudioWorkGroup getAudioWorkGroup() const override
  {
    return m_driver->getAudioWorkGroup();
  }
  muse::async::Notification currentWorkgroupChanged() const override
  {
    return m_driver->currentWorkgroupChanged();
  }
  bool open(const Spec &spec, Spec *activeSpec) override;
  void close() override;
  bool isOpened() const override { return m_driver->isOpened(); }
  const Spec &activeSpec() const override { return m_spec; }
  muse::async::Channel<Spec> activeSpecChanged() const override
  {
    return m_activeSpecChanged;
  }
  std::vector<muse::audio::samples_t>
  availableOutputDeviceBufferSizes() const override
  {
    return m_driver->availableOutputDeviceBufferSizes();
  }
  std::vector<muse::audio::sample_rate_t>
  availableOutputDeviceSampleRates() const override
  {
    return m_driver->availableOutputDeviceSampleRates();
  }
  muse::audio::AudioDeviceList availableOutputDevices() const override
  {
    return m_driver->availableOutputDevices();
  }
  muse::async::Notification availableOutputDevicesChanged() const override
  {
    return m_availableOutputDevicesChanged;
  }

private:
  /**
   * The driver's active spec, naming the device as it was asked for.
   */
  Spec asAsked(Spec spec) const;
  void onDeviceChanged();

  const muse::audio::IAudioDriverPtr m_driver;
  /**
   * As last given to open(): reopening, it is asked again.
   */
  Spec m_asked;
  Spec m_spec;
  muse::async::Channel<Spec> m_activeSpecChanged;
  muse::async::Notification m_availableOutputDevicesChanged;
  std::shared_ptr<void> m_watch;
};

/**
 * Makes the drivers the given factory makes, following the device.
 */
class DeviceFollowingAudioDriverFactory
    : public muse::audio::IAudioDriverFactory
{
public:
  explicit DeviceFollowingAudioDriverFactory(
      std::shared_ptr<muse::audio::IAudioDriverFactory> factory)
      : m_factory{std::move(factory)}
  {
  }
  std::vector<std::string> availableAudioDrivers() const override
  {
    return m_factory->availableAudioDrivers();
  }
  muse::audio::IAudioDriverPtr
  createDriver(const std::string &name) const override;

private:
  const std::shared_ptr<muse::audio::IAudioDriverFactory> m_factory;
};
} // namespace dgk
