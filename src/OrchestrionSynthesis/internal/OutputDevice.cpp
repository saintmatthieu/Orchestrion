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

// The platforms that can't tell (yet): see OutputDevice.mm for macOS.
namespace dgk::outputDevice
{
std::optional<muse::audio::AudioDeviceID> systemDefault() { return {}; }

std::optional<muse::audio::sample_rate_t>
sampleRate(const muse::audio::AudioDeviceID &)
{
  return {};
}

std::shared_ptr<void> watch(const muse::audio::AudioDeviceID &,
                            std::function<void()>)
{
  return nullptr;
}
} // namespace dgk::outputDevice
