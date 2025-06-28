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
#include <async/channel.h>

namespace dgk
{
class PromisedSynthesizer : public IOrchestrionSynthesizer,
                            public muse::async::Asyncable
{
public:
  using SynthPromise =
      muse::async::Channel<std::shared_ptr<IOrchestrionSynthesizer>>;

  PromisedSynthesizer(SynthPromise promise);

  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const TrackIndex* channels, const int *pitches,
                 const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const TrackIndex* channels,
                  const int *pitches) override;
  void onPedal(bool on) override;

private:
  SynthPromise m_promise;
  std::shared_ptr<IOrchestrionSynthesizer> m_synthesizer;
};
} // namespace dgk