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

#include <optional>

namespace dgk
{
/**
 * The rate PipeWire's graph runs at (outputDevice::sampleRate on Linux):
 * `clock.force-rate` if set, else `clock.rate` (the "settings" metadata, what
 * `pw-metadata -n settings` shows). A device whose driver runs at another rate
 * isn't told apart: the graph's is what every sink runs at but where configured
 * otherwise. Asks a connection of its own (a few ms) and gives up after half a
 * second; nothing without a PipeWire server.
 */
std::optional<muse::audio::sample_rate_t> pipeWireGraphRate();
} // namespace dgk
