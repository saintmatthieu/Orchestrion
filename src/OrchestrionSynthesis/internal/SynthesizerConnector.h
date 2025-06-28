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
#include <async/asyncable.h>
#include <audio/internal/worker/iaudioengine.h>
#include <audio/iplayback.h>
#include <audio/isynthresolver.h>
#include <context/iglobalcontext.h>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>

namespace dgk
{
class OrchestrionSynthResolver;

class SynthesizerConnector : public ISynthesizerConnector,
                             public muse::async::Asyncable,
                             public muse::Injectable
{
  muse::Inject<muse::audio::IAudioEngine> audioEngine;
  muse::Inject<muse::audio::synth::ISynthResolver> synthResolver;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<muse::audio::IPlayback> playback;
  muse::Inject<mu::playback::IPlaybackController> playbackController;

public:
  SynthesizerConnector();
  void onInit();
  void onAllInited();

private:
  void connectVstInstrument(const muse::audio::AudioResourceId &) override;
  void connectFluidSynth() override;
  void disconnect() override;
  void setInputParams();
  void setOutputParams();

  const std::shared_ptr<OrchestrionSynthResolver> m_orchestrionSynthResolver;
  std::optional<muse::audio::TrackSequenceId> m_trackSequenceId;
  std::vector<std::pair<muse::audio::TrackSequenceId, muse::audio::TrackId>>
      m_tracks;
};
} // namespace dgk