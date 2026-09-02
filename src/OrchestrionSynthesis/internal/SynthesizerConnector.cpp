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
#include "SynthesizerConnector.h"
#include "OrchestrionSynthResolver.h"

#include <algorithm>
#include <log.h>

namespace dgk
{
SynthesizerConnector::SynthesizerConnector()
    : m_orchestrionSynthResolver{std::make_shared<OrchestrionSynthResolver>()},
      m_synth{OrchestrionSynthResolver::fluidSynthValue}
{
}

// The input params that route a track to Orchestrion's resolver (registered
// for the built-in source type) and tell it which synthesizer is wanted.
muse::audio::AudioSourceParams SynthesizerConnector::inputParams() const
{
  muse::audio::AudioSourceParams params;
  params.resourceMeta.id = "Orchestrion Synth Resolver";
  params.resourceMeta.type = muse::audio::resourceTypeName(
      muse::audio::AudioResourceType::FluidSoundfont);
  params.resourceMeta.vendor = "saintmatthieu";
  params.resourceMeta.attributes[OrchestrionSynthResolver::synthAttribute] =
      m_synth;
  if (!m_vstId.empty())
    params.resourceMeta.attributes[OrchestrionSynthResolver::vstIdAttribute] =
        m_vstId;
  return params;
}

void SynthesizerConnector::onAllInited()
{
  playback()->trackAdded().onReceive(this, [this](muse::audio::TrackId trackId)
                                     { m_tracks.push_back(trackId); });
  playback()->trackRemoved().onReceive(
      this,
      [this](muse::audio::TrackId trackId)
      {
        m_tracks.erase(std::remove(m_tracks.begin(), m_tracks.end(), trackId),
                       m_tracks.end());
      });

  // Take over the built-in source type from MuseScore's FluidSynth resolver.
  // (Registered after the audio engine has started too, in case the engine's
  // setup re-registered the built-in one.)
  registerResolver();
  startAudioController()->isAudioStartedChanged().onReceive(
      this,
      [this](bool started)
      {
        if (started)
          registerResolver();
      });

  playbackController()->isPlayAllowedChanged().onReceive(
      this,
      [this](bool allowed)
      {
        if (allowed)
        {
          setInputParams();
          setOutputParams();
        }
      });
}

void SynthesizerConnector::registerResolver()
{
  synthResolver()->registerResolver(muse::audio::AudioSourceType::Fluid,
                                    m_orchestrionSynthResolver);
}

void SynthesizerConnector::connectVstInstrument(
    const muse::audio::AudioResourceId &id)
{
  m_synth = OrchestrionSynthResolver::vstSynthValue;
  m_vstId = muse::String::fromStdString(id);
  setInputParams();
}

void SynthesizerConnector::connectFluidSynth()
{
  m_synth = OrchestrionSynthResolver::fluidSynthValue;
  m_vstId.clear();
  setInputParams();
}

void SynthesizerConnector::disconnect()
{
  m_synth = OrchestrionSynthResolver::noSynthValue;
  m_vstId.clear();
  setInputParams();
}

void SynthesizerConnector::setInputParams()
{
  const muse::audio::AudioSourceParams params = inputParams();
  LOGI() << "Routing " << m_tracks.size()
         << " track(s) to the Orchestrion synthesizer";
  for (const muse::audio::TrackId trackId : m_tracks)
    playback()->setSourceParams(trackId, params);
}

void SynthesizerConnector::setOutputParams()
{
  // Keep things under control, disabling reverb and other effects.
  for (const muse::audio::TrackId trackId : m_tracks)
  {
    playback()->setFxChainParams(trackId, muse::audio::AudioFxChain{});
    playback()->setAuxSendsParams(trackId, muse::audio::AuxSendsParams{});
  }
}
} // namespace dgk
