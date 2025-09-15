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
#include "PolyphonicSynthesizerImpl.h"
#include <audio/audiotypes.h>
#include <audio/worker/isynthesizer.h>
#include <vst/internal/vstplugininstance.h>

namespace muse::vst
{
class VstAudioClient;
}
namespace dgk
{
class OrchestrionVstSynthesizer : public IOrchestrionSynthesizer,
                                  private PolyphonicSynthesizerImpl
{
public:
  OrchestrionVstSynthesizer(
      std::shared_ptr<muse::vst::VstPluginInstance> loadedVstPlugin,
      int sampleRate);

private:
  int sampleRate() const override;
  size_t process(float *buffer, size_t samplesPerChannel) override;
  void onNoteOns(size_t numNoteons, const TrackIndex *channels,
                 const int *pitches, const float *velocities) override;
  void onNoteOffs(size_t numNoteoffs, const TrackIndex *channels,
                  const int *pitches) override;
  void onPedal(bool on) override;
  void allNotesOff() override;

  const int m_sampleRate;
  std::unique_ptr<muse::vst::VstAudioClient> m_vstAudioClient;
};
} // namespace dgk