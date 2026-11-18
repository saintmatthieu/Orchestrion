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

#include <audio/iaudiodriver.h>
#include <audio/main/internal/iaudiodriverfactory.h>

namespace dgk
{
/**
 * The platform's driver, its callbacks timed for the LatencyTrace: when each
 * starts and ends, and how many frames it asks for.
 */
class TracingAudioDriver : public muse::audio::IAudioDriver
{
public:
  /**
   * `name`: the driver's, as the factory was asked for it (two drivers may
   * call themselves the same, as macOS' CoreAudio ones do).
   */
  TracingAudioDriver(muse::audio::IAudioDriverPtr driver, std::string name);

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
  void close() override { m_driver->close(); }
  bool isOpened() const override { return m_driver->isOpened(); }
  const Spec &activeSpec() const override { return m_driver->activeSpec(); }
  muse::async::Channel<Spec> activeSpecChanged() const override
  {
    return m_driver->activeSpecChanged();
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
    return m_driver->availableOutputDevicesChanged();
  }

private:
  const muse::audio::IAudioDriverPtr m_driver;
  const std::string m_name;
  int m_opens = 0;
};

/**
 * Makes the drivers the given factory makes, traced.
 */
class TracingAudioDriverFactory : public muse::audio::IAudioDriverFactory
{
public:
  explicit TracingAudioDriverFactory(
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
