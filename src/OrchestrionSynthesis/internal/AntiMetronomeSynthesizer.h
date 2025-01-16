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

#include "IOrchestrionSynthesizer.h"
#include <async/asyncable.h>
#include <functional>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>

namespace dgk
{
class AntiMetronomeSynthesizer : public IOrchestrionSynthesizer,
                                 public muse::Injectable,
                                 public muse::async::Asyncable
{
public:
  using SynthesizerFactory =
      std::function<std::unique_ptr<IOrchestrionSynthesizer>(int sampleRate)>;

  AntiMetronomeSynthesizer(int sampleRate, muse::audio::TrackId,
                           SynthesizerFactory);

  // IOrchestrionSynthesizer
public:
  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const int *pitches) override;
  void onPedal(bool on) override;

private:
  void SetOrResetSynth();

  muse::Inject<mu::playback::IPlaybackController> playbackController;
  const int m_sampleRate;
  const muse::audio::TrackId m_trackId;
  const SynthesizerFactory m_factory;
  std::unique_ptr<IOrchestrionSynthesizer> m_synthesizer;
  muse::mpe::PlaybackData m_playbackData;
};
} // namespace dgk