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

#include <ISynthesizerConnector.h>
#include <async/asyncable.h>
#include <audio/isynthresolver.h>
#include <modularity/ioc.h>
#include <optional>

namespace dgk
{
class OrchestrionSynthResolver
    : public muse::audio::synth::ISynthResolver::IResolver,
      public muse::Injectable,
      public muse::async::Asyncable
{
public:
  void resolveToVst(const muse::audio::AudioResourceId &);
  void resolveToFluid();
  void resolveToNone();

private:
  muse::Inject<ISynthesizerConnector> synthesizerConnector;

private:
  muse::audio::synth::ISynthesizerPtr
  resolveSynth(const muse::audio::TrackId,
               const muse::audio::AudioInputParams &) const override;

  bool
  hasCompatibleResources(const muse::audio::PlaybackSetupData &) const override;

  muse::audio::AudioResourceMetaList resolveResources() const override;

  muse::audio::SoundPresetList
  resolveSoundPresets(const muse::audio::AudioResourceMeta &) const override;

  void refresh() override;

  void clearSources() override;

  enum class SynthType
  {
    None,
    Vst,
    Fluid
  };

  SynthType m_synthType = SynthType::None;
  std::optional<muse::audio::AudioResourceId> m_vstId;
};
} // namespace dgk