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
#include "ITrackChannelMapper.h"
#include "TrackAudioInput.h"

namespace dgk
{
class IOrchestrionSynthesizer;

class FluidTrackAudioInput : public TrackAudioInput
{
  muse::Inject<ITrackChannelMapper> mapper;

public:
  FluidTrackAudioInput();

private:
  void processEvent(const EventVariant &event) override;
  bool _isActive() const override;
  void _setIsActive(bool arg) override;
  void _setSampleRate(unsigned int sampleRate) override;
  muse::audio::samples_t
  _process(float *buffer, muse::audio::samples_t samplesPerChannel) override;

  void sendNoteoffs(const NoteEvent *noteoffs, size_t numNoteoffs);
  void sendNoteons(const NoteEvent *noteons, size_t numNoteons);

  uint64_t m_sampleRate = 0;
  std::vector<std::unique_ptr<IOrchestrionSynthesizer>> m_synthesizers;
  std::vector<float> m_mixBuffer;
  size_t m_maxSamples;
};
} // namespace dgk