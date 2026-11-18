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
#include "OutputDevice.h"

#include <Block.h>
#include <CoreAudio/CoreAudio.h>
#include <dispatch/dispatch.h>

#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>

namespace dgk::outputDevice
{
namespace
{
constexpr AudioObjectPropertyAddress defaultOutputAddress{
    kAudioHardwarePropertyDefaultOutputDevice,
    kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};

constexpr AudioObjectPropertyAddress nominalRateAddress{
    kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain};

std::optional<AudioObjectID> defaultObject()
{
  AudioObjectID object = kAudioObjectUnknown;
  UInt32 size = sizeof(object);
  if (AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                 &defaultOutputAddress, 0, nullptr, &size,
                                 &object) != noErr ||
      object == kAudioObjectUnknown)
    return {};
  return object;
}

std::optional<AudioObjectID> object(const muse::audio::AudioDeviceID &device)
{
  if (device.empty() || device == muse::audio::DEFAULT_DEVICE_ID)
    return defaultObject();
  char *end = nullptr;
  const unsigned long id = std::strtoul(device.c_str(), &end, 10);
  if (end == device.c_str() || *end != '\0')
    return {};
  return static_cast<AudioObjectID>(id);
}

/**
 * Listens to a property for as long as it lives, on the main queue. The block
 * holds its own copy of the callback: one already queued when the listener is
 * removed doesn't outlive what it calls.
 */
class Listener
{
public:
  Listener(AudioObjectID object, const AudioObjectPropertyAddress &address,
           std::function<void()> onChange)
      : m_object{object}, m_address{address}
  {
    // Copied off the stack (no ARC here), so that removing it gives
    // CoreAudio the very block it was given
    m_block = Block_copy(^(UInt32, const AudioObjectPropertyAddress *) {
      onChange();
    });
    m_added = AudioObjectAddPropertyListenerBlock(m_object, &m_address,
                                                  dispatch_get_main_queue(),
                                                  m_block) == noErr;
  }

  ~Listener()
  {
    if (m_added)
      AudioObjectRemovePropertyListenerBlock(
          m_object, &m_address, dispatch_get_main_queue(), m_block);
    Block_release(m_block);
  }

  Listener(const Listener &) = delete;
  Listener &operator=(const Listener &) = delete;

private:
  const AudioObjectID m_object;
  const AudioObjectPropertyAddress m_address;
  AudioObjectPropertyListenerBlock m_block = nullptr;
  bool m_added = false;
};

struct Watch
{
  std::unique_ptr<Listener> defaultDevice;
  std::unique_ptr<Listener> rate;
};
} // namespace

std::optional<muse::audio::AudioDeviceID> systemDefault()
{
  if (const auto object = defaultObject())
    return std::to_string(*object);
  return {};
}

std::optional<muse::audio::sample_rate_t>
sampleRate(const muse::audio::AudioDeviceID &device)
{
  const auto id = object(device);
  if (!id)
    return {};
  Float64 rate = 0;
  UInt32 size = sizeof(rate);
  if (AudioObjectGetPropertyData(*id, &nominalRateAddress, 0, nullptr, &size,
                                 &rate) != noErr ||
      rate <= 0)
    return {};
  return static_cast<muse::audio::sample_rate_t>(std::lround(rate));
}

std::shared_ptr<void> watch(const muse::audio::AudioDeviceID &device,
                            std::function<void()> onChange)
{
  auto watch = std::make_shared<Watch>();
  watch->defaultDevice = std::make_unique<Listener>(
      kAudioObjectSystemObject, defaultOutputAddress, onChange);
  if (const auto id = object(device))
    watch->rate =
        std::make_unique<Listener>(*id, nominalRateAddress, std::move(onChange));
  return watch;
}
} // namespace dgk::outputDevice
