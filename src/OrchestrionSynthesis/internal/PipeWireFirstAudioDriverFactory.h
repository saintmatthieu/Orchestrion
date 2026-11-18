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

#include <audio/main/internal/iaudiodriverfactory.h>

#include <memory>

namespace dgk
{
/**
 * Linux: PipeWire's own client where ALSA is asked for, which MuseScore's
 * audio driver setting is by default (Orchestrion has no UI for it). Through
 * ALSA, a PipeWire system plays at its graph's quantum; PipeWire's client
 * asks for a latency (node.latency), and the graph's quantum comes down to
 * the lowest asked. The factory makes an ALSA driver anyway without a
 * PipeWire server, or with MUSESCORE_FORCE_ALSA set.
 *
 * MuseScore sets its default and opens the driver in the same call
 * (AudioModule::onInit): no other module gets to change the default in
 * between, hence the factory.
 */
class PipeWireFirstAudioDriverFactory : public muse::audio::IAudioDriverFactory
{
public:
  explicit PipeWireFirstAudioDriverFactory(
      std::shared_ptr<muse::audio::IAudioDriverFactory> factory)
      : m_factory{std::move(factory)}
  {
  }

  std::vector<std::string> availableAudioDrivers() const override
  {
    return m_factory->availableAudioDrivers();
  }

  muse::audio::IAudioDriverPtr
  createDriver(const std::string &name) const override
  {
    return m_factory->createDriver(name == "ALSA" ? "PipeWire" : name);
  }

private:
  const std::shared_ptr<muse::audio::IAudioDriverFactory> m_factory;
};
} // namespace dgk
