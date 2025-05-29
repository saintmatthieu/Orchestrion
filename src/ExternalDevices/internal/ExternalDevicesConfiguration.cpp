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
#include "ExternalDevicesConfiguration.h"

namespace dgk
{
namespace
{
const std::string module_name("ExternalDevices");
const muse::Settings::Key MIDI_DEVICE(module_name, "MIDI_DEVICE");
const muse::Settings::Key AUDIO_DEVICE(module_name, "AUDIO_DEVICE");
} // namespace

void ExternalDevicesConfiguration::init()
{
  muse::settings()->setDefaultValue(MIDI_DEVICE, muse::Val{""});
  muse::settings()->setDefaultValue(AUDIO_DEVICE, muse::Val{""});
}

std::optional<ExternalDeviceId>
ExternalDevicesConfiguration::readSelectedMidiDevice() const
{
  return readSelectedDevice(MIDI_DEVICE);
}

std::optional<ExternalDeviceId>
ExternalDevicesConfiguration::readSelectedAudioDevice() const
{
  return readSelectedDevice(AUDIO_DEVICE);
}

void ExternalDevicesConfiguration::writeSelectedMidiDevice(
    const std::optional<ExternalDeviceId> &deviceId)
{
  writeSelectedDevice(MIDI_DEVICE, deviceId);
}

void ExternalDevicesConfiguration::writeSelectedAudioDevice(
    const std::optional<ExternalDeviceId> &deviceId)
{
  writeSelectedDevice(AUDIO_DEVICE, deviceId);
}

std::optional<ExternalDeviceId>
ExternalDevicesConfiguration::readSelectedDevice(
    const muse::Settings::Key &key) const
{
  const std::string value = muse::settings()->value(key).toString();
  if (value.empty())
    return std::nullopt;
  return ExternalDeviceId{value};
}

void ExternalDevicesConfiguration::writeSelectedDevice(
    const muse::Settings::Key &key,
    const std::optional<ExternalDeviceId> &deviceId)
{
  const muse::Val value{deviceId.value_or(ExternalDeviceId{""}).value};
  muse::settings()->setLocalValue(key, value);
}
} // namespace dgk