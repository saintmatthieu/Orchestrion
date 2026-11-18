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

#include <muse_framework_config.h>

#ifdef MUSE_MODULE_AUDIO_PIPEWIRE
#include "PipeWireGraphRate.h"
#endif

// Windows and Linux (see OutputDevice.mm for macOS). On Linux with a PipeWire
// server, the rate of its graph, whatever the device: PipeWire's client,
// asked for none, follows the default device itself (and so does ALSA's
// "default" through PipeWire's plugin); a rate change isn't watched yet.
namespace dgk::outputDevice
{
std::optional<muse::audio::AudioDeviceID> systemDefault() { return {}; }

std::optional<muse::audio::sample_rate_t>
sampleRate(const muse::audio::AudioDeviceID &)
{
#ifdef MUSE_MODULE_AUDIO_PIPEWIRE
  return pipeWireGraphRate();
#else
  return {};
#endif
}

std::shared_ptr<void> watch(const muse::audio::AudioDeviceID &,
                            std::function<void()>)
{
  return nullptr;
}
} // namespace dgk::outputDevice
