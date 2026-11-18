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
#include "DeviceFollowingAudioDriver.h"
#include "OutputDevice.h"

#include <utility>

#include <log.h>

namespace dgk
{
namespace
{
bool isDefault(const muse::audio::AudioDeviceID &device)
{
  return device.empty() || device == muse::audio::DEFAULT_DEVICE_ID;
}
} // namespace

DeviceFollowingAudioDriver::DeviceFollowingAudioDriver(
    muse::audio::IAudioDriverPtr driver)
    : m_driver{std::move(driver)}
{
  m_driver->activeSpecChanged().onReceive(this,
                                          [this](const Spec &spec)
                                          {
                                            m_spec = asAsked(spec);
                                            m_activeSpecChanged.send(m_spec);
                                          });
  m_driver->availableOutputDevicesChanged().onNotify(
      this, [this] { m_availableOutputDevicesChanged.notify(); });
}

DeviceFollowingAudioDriver::Spec
DeviceFollowingAudioDriver::asAsked(Spec spec) const
{
  // Only where the default can be followed
  if (isDefault(m_asked.deviceId) && !spec.deviceId.empty() &&
      outputDevice::systemDefault())
    spec.deviceId = muse::audio::DEFAULT_DEVICE_ID;
  return spec;
}

bool DeviceFollowingAudioDriver::open(const Spec &spec, Spec *activeSpec)
{
  m_asked = spec;
  Spec atDeviceRate = spec;
  if (const auto rate = outputDevice::sampleRate(spec.deviceId))
  {
    if (*rate != spec.output.sampleRate)
      LOGI() << "Opening " << spec.deviceId << " at the rate it runs at, "
             << *rate << " Hz, rather than " << spec.output.sampleRate;
    atDeviceRate.output.sampleRate = *rate;
  }

  const bool opened = m_driver->open(atDeviceRate, nullptr);
  m_spec = asAsked(m_driver->activeSpec());
  m_watch.reset();
  if (!opened)
    return false;

  m_watch = outputDevice::watch(m_driver->activeSpec().deviceId,
                                [weak = weak_from_this()]
                                {
                                  if (const auto self = weak.lock())
                                    self->onDeviceChanged();
                                });
  if (activeSpec)
    *activeSpec = m_spec;
  return true;
}

void DeviceFollowingAudioDriver::close()
{
  m_watch.reset();
  m_driver->close();
  m_spec = asAsked(m_driver->activeSpec());
}

void DeviceFollowingAudioDriver::onDeviceChanged()
{
  if (!m_driver->isOpened())
    return;

  const Spec &active = m_driver->activeSpec();
  bool changed = false;
  if (isDefault(m_asked.deviceId) && !isDefault(active.deviceId))
    if (const auto device = outputDevice::systemDefault();
        device && *device != active.deviceId)
    {
      LOGI() << "The system's default output device is now " << *device
             << " (was " << active.deviceId << ")";
      changed = true;
    }
  if (const auto rate = outputDevice::sampleRate(active.deviceId);
      rate && *rate != active.output.sampleRate)
  {
    LOGI() << "Output device " << active.deviceId << " now runs at " << *rate
           << " Hz (was " << active.output.sampleRate << ")";
    changed = true;
  }
  if (changed)
    m_availableOutputDevicesChanged.notify();
}

muse::audio::IAudioDriverPtr
DeviceFollowingAudioDriverFactory::createDriver(const std::string &name) const
{
  muse::audio::IAudioDriverPtr driver = m_factory->createDriver(name);
  if (!driver)
    return driver;
  return std::make_shared<DeviceFollowingAudioDriver>(std::move(driver));
}
} // namespace dgk
