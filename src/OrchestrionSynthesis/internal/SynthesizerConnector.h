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

#include "ISynthesizerConnector.h"
#include "OrchestrionCommon/OrchestrionIoc.h"

#include <async/asyncable.h>
#include <audio/engine/isynthresolver.h>
#include <audio/main/iplayback.h>
#include <audio/main/istartaudiocontroller.h>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>

#include <memory>
#include <vector>

namespace dgk
{
class OrchestrionSynthResolver;

/**
 * Plugs Orchestrion's synthesizer resolver into MuseScore's audio engine and
 * points the score's tracks at it.
 */
class SynthesizerConnector : public ISynthesizerConnector,
                             public muse::async::Asyncable,
                             public dgk::Injectable
{
  dgk::Inject<muse::audio::synth::ISynthResolver> synthResolver{this};
  dgk::Inject<muse::audio::IStartAudioController> startAudioController{this};
  dgk::Inject<muse::audio::IPlayback> playback{this};
  dgk::Inject<mu::playback::IPlaybackController> playbackController{this};

public:
  SynthesizerConnector();
  void onAllInited();

private:
  void connectVstInstrument(const muse::audio::AudioResourceId &) override;
  void connectFluidSynth() override;
  void disconnect() override;

  void registerResolver();
  void setInputParams();
  void setOutputParams();
  muse::audio::AudioSourceParams inputParams() const;

  const std::shared_ptr<OrchestrionSynthResolver> m_orchestrionSynthResolver;
  std::vector<muse::audio::TrackId> m_tracks;
  // The selected synthesizer (built-in by default), see OrchestrionSynthResolver.
  muse::String m_synth;
  muse::String m_vstId;
};
} // namespace dgk
