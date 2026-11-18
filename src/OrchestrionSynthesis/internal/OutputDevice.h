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

#include <audio/common/audiotypes.h>

#include <functional>
#include <memory>
#include <optional>

/**
 * What the system tells of its output devices, where the platform can tell
 * (macOS so far: devices are named by their CoreAudio ids, as both CoreAudio
 * drivers name them; elsewhere, nothing).
 */
namespace dgk::outputDevice
{
/**
 * The id of the system's default output device.
 */
std::optional<muse::audio::AudioDeviceID> systemDefault();

/**
 * The rate the device runs at (`DEFAULT_DEVICE_ID` or empty: the system's
 * default).
 */
std::optional<muse::audio::sample_rate_t>
sampleRate(const muse::audio::AudioDeviceID &device);

/**
 * Calls `onChange` on the main thread when the system's default output device
 * changes, or the rate the given device runs at, until the returned object is
 * destroyed. Null where the platform can't tell.
 */
std::shared_ptr<void> watch(const muse::audio::AudioDeviceID &device,
                            std::function<void()> onChange);
} // namespace dgk::outputDevice
