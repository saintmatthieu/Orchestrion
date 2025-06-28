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
#include <async/async.h>
#include <audio/iaudiooutput.h>
#include <audio/internal/audiothread.h>
#include <audio/itracks.h>

namespace dgk
{
SynthesizerConnector::SynthesizerConnector()
    : m_orchestrionSynthResolver{std::make_shared<OrchestrionSynthResolver>()}
{
}

void SynthesizerConnector::onAllInited()
{
  const auto tracks = playback()->tracks();
  tracks->trackAdded().onReceive(
      this, [this](muse::audio::TrackSequenceId sequenceId,
                   muse::audio::TrackId trackId)
      { m_tracks.emplace_back(std::move(sequenceId), std::move(trackId)); });
  tracks->trackRemoved().onReceive(
      this,
      [this](muse::audio::TrackSequenceId sequenceId,
             muse::audio::TrackId trackId)
      {
        m_tracks.erase(std::remove_if(m_tracks.begin(), m_tracks.end(),
                                      [sequenceId, trackId](const auto &pair)
                                      {
                                        return pair.first == sequenceId &&
                                               pair.second == trackId;
                                      }),
                       m_tracks.end());
      });

  muse::async::Async::call(
      this,
      [this]
      {
        muse::audio::AudioSourceParams defaultParams =
            synthResolver()->resolveDefaultInputParams();

        auto &meta = defaultParams.resourceMeta;
        meta.id = "Orchestrion Synth Resolver";
        meta.type = muse::audio::AudioResourceType::FluidSoundfont;
        meta.vendor = "saintmatthieu";
        meta.attributes.clear();

        synthResolver()->init(defaultParams);
        synthResolver()->registerResolver(muse::audio::AudioSourceType::Fluid,
                                          m_orchestrionSynthResolver);
      },
      muse::audio::AudioThread::ID);

  playbackController()->isPlayAllowedChanged().onNotify(
      this,
      [this]
      {
        if (playbackController()->isPlayAllowed())
        {
          setInputParams();
          setOutputParams();
        }
      });
}

void SynthesizerConnector::connectVstInstrument(
    const muse::audio::AudioResourceId &id)
{
  m_orchestrionSynthResolver->resolveToVst(id);
  setInputParams();
}

void SynthesizerConnector::connectFluidSynth()
{
  m_orchestrionSynthResolver->resolveToFluid();
  setInputParams();
}

void SynthesizerConnector::disconnect()
{
  m_orchestrionSynthResolver->resolveToNone();
  setInputParams();
}

void SynthesizerConnector::setInputParams()
{
  muse::async::Async::call(
      this,
      [this]
      {
        const auto params = synthResolver()->resolveDefaultInputParams();
        const auto tracks = playback()->tracks();
        for (const auto &[sequenceId, trackId] : m_tracks)
          tracks->setInputParams(sequenceId, trackId, params);
      },
      muse::audio::AudioThread::ID);
}

void SynthesizerConnector::setOutputParams()
{
  muse::async::Async::call(
      this,
      [this]
      {
        const auto tracks = playback()->tracks();
        const muse::audio::IAudioOutputPtr output = playback()->audioOutput();
        // Keep things under control, disabling reverb and other effects.
        const muse::audio::AudioOutputParams outParams{};
        for (const auto &[sequenceId, trackId] : m_tracks)
          output->setOutputParams(sequenceId, trackId, outParams);
      },
      muse::audio::AudioThread::ID);
}
} // namespace dgk