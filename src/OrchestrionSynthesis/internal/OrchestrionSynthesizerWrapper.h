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
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"
#include <async/asyncable.h>
#include <audio/isynthesizer.h>
#include <modularity/ioc.h>

namespace dgk
{
class IOrchestrionSynthesizer;

class OrchestrionSynthesizerWrapper : public muse::audio::synth::ISynthesizer,
                                      public muse::Injectable,
                                      public muse::async::Asyncable
{
public:
  using SynthFactory = std::function<std::unique_ptr<IOrchestrionSynthesizer>(
      unsigned int sampleRate)>;

  OrchestrionSynthesizerWrapper(SynthFactory);

  // ISynthesizer
private:
  std::string name() const override;
  muse::audio::AudioSourceType type() const override;
  bool isValid() const override;

  void setup(const muse::mpe::PlaybackData &playbackData) override;
  const muse::mpe::PlaybackData &playbackData() const override;

  const muse::audio::AudioInputParams &params() const override;
  muse::async::Channel<muse::audio::AudioInputParams>
  paramsChanged() const override;

  muse::audio::msecs_t playbackPosition() const override;
  void setPlaybackPosition(const muse::audio::msecs_t newPosition) override;

  void revokePlayingNotes() override;
  void flushSound() override;

  // IAudioSource
private:
  bool isActive() const override;
  void setIsActive(bool arg) override;
  void setSampleRate(unsigned int sampleRate) override;
  unsigned int audioChannelsCount() const override;
  muse::async::Channel<unsigned int> audioChannelsCountChanged() const override;
  muse::audio::samples_t
  process(float *buffer, muse::audio::samples_t samplesPerChannel) override;

private:
  void setupCallback(const IOrchestrionSequencer &sequencer);
  void processEvent(const EventVariant &event);
  void sendNoteoffs(const NoteEvent *noteoffs, size_t numNoteoffs);
  void sendNoteons(const NoteEvent *noteons, size_t numNoteons);

  muse::Inject<IOrchestrion> orchestrion;

  muse::audio::AudioSourceParams m_params;
  muse::async::Channel<muse::audio::AudioInputParams> m_paramsChanged;
  muse::audio::msecs_t m_playbackPosition = 0;
  muse::async::Channel<unsigned int> m_audioChannelsCountChanged;
  bool m_isActive = true;

  const SynthFactory m_synthFactory;
  unsigned m_sampleRate = 0;
  std::unique_ptr<IOrchestrionSynthesizer> m_synthesizer;
};
} // namespace dgk