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
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"

#include <async/asyncable.h>
#include <audio/common/audiotypes.h>
#include <audio/engine/isynthesizer.h>
#include <modularity/ioc.h>

#include <functional>
#include <memory>

namespace dgk
{
class IOrchestrionSynthesizer;

/**
 * Adapts an IOrchestrionSynthesizer to MuseScore's audio engine as an
 * ISynthesizer. The engine drives process(); the note events come straight from
 * Orchestrion's sequencer, not from the engine's playback data.
 */
class OrchestrionSynthesizerWrapper : public muse::audio::synth::ISynthesizer,
                                      public dgk::Injectable,
                                      public muse::async::Asyncable
{
public:
  using SynthFactory = std::function<std::unique_ptr<IOrchestrionSynthesizer>(
      const muse::audio::OutputSpec &spec)>;

  OrchestrionSynthesizerWrapper(SynthFactory,
                                muse::audio::AudioInputParams params);

  // ISynthesizer
private:
  std::string name() const override;
  muse::audio::AudioSourceType type() const override;
  bool isValid() const override;
  void setMode(const muse::audio::ProcessMode mode) override;
  muse::audio::ProcessMode mode() const override;
  void setOutputSpec(const muse::audio::OutputSpec &spec) override;
  void setup(const muse::mpe::PlaybackData &playbackData) override;
  const muse::mpe::PlaybackData &playbackData() const override;
  const muse::audio::AudioInputParams &params() const override;
  muse::async::Channel<muse::audio::AudioInputParams>
  paramsChanged() const override;
  muse::audio::TimePosition playbackPosition() const override;
  void setPlaybackPosition(const muse::audio::TimePosition &position) override;
  void prepareToPlay() override;
  bool readyToPlay() const override;
  muse::async::Notification readyToPlayChanged() const override;
  void flushSound() override;
  bool hasPendingChunks() const override;
  void processInput() override;
  muse::audio::InputProcessingProgress inputProcessingProgress() const override;
  void clearCache() override;
  muse::audio::samples_t
  process(float *buffer, muse::audio::samples_t samplesPerChannel) override;

private:
  void setupCallback(const IOrchestrionSequencer &sequencer);
  void processEvent(const EventVariant &event);
  void sendNoteoffs(const NoteEvent *noteoffs, size_t numNoteoffs);
  void sendNoteons(const NoteEvent *noteons, size_t numNoteons);

  dgk::Inject<IOrchestrion> orchestrion{this};

  muse::audio::AudioSourceParams m_params;
  muse::async::Channel<muse::audio::AudioInputParams> m_paramsChanged;
  muse::async::Notification m_readyToPlayChanged;
  muse::audio::ProcessMode m_mode = muse::audio::ProcessMode::Undefined;
  muse::audio::TimePosition m_playbackPosition;
  const SynthFactory m_synthFactory;
  muse::audio::OutputSpec m_outputSpec;
  std::unique_ptr<IOrchestrionSynthesizer> m_synthesizer;
};
} // namespace dgk
