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
#pragma once

#include "ISynthesizerManager.h"
#include "OrchestrionSynthesis/ISynthesizerConnector.h"
#include <async/asyncable.h>
#include <audio/iplayback.h>
#include <audioplugins/iknownaudiopluginsregister.h>
#include <midi/imidioutport.h>
#include <modularity/ioc.h>

namespace dgk
{
class SynthesizerManager : public ISynthesizerManager,
                           public muse::Injectable,
                           public muse::async::Asyncable
{
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort = {this};
  muse::Inject<muse::audioplugins::IKnownAudioPluginsRegister> knownPlugins = {
      this};
  muse::Inject<muse::audio::IPlayback> playback = {this};
  muse::Inject<ISynthesizerConnector> synthesizerConnector = {this};

public:
  SynthesizerManager() = default;
  void init();
  void onAllInited();

  // ISynthesizerManager
private:
  std::vector<DeviceDesc> availableSynths() const override;
  muse::async::Notification availableSynthsChanged() const override;
  bool selectSynth(const std::string &synthId) override;
  muse::async::Notification selectedSynthChanged() const override;
  std::string selectedSynth() const override;

  std::vector<muse::audioplugins::AudioPluginInfo> availableInstruments() const;

  muse::async::Notification m_availableSynthsChanged;
  muse::async::Notification m_selectedSynthChanged;
  std::vector<muse::audioplugins::AudioPluginInfo> m_availableSynths;
  std::optional<std::string> m_selectedSynth;
};
} // namespace dgk