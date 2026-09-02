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

#include "OrchestrionCommon/OrchestrionIoc.h"

#include <async/asyncable.h>
#include <audio/engine/isynthresolver.h>
#include <modularity/ioc.h>
#include <vst/ivstinstancesregister.h>

#include <global/types/string.h>

namespace dgk
{
/**
 * Resolves the audio engine's synthesizer requests for Orchestrion's tracks
 * to Orchestrion's own synthesizers (the built-in FluidSynth-based one or a
 * VST instrument), driven by the gesture sequencer.
 *
 * Which synthesizer is wanted is carried by the track's source params (see
 * the attributes below), so that changing the selection changes the params
 * and makes the engine re-resolve the track's synthesizer.
 */
class OrchestrionSynthResolver
    : public muse::audio::synth::ISynthResolver::IResolver,
      public dgk::Injectable,
      public muse::async::Asyncable
{
public:
  static const muse::String synthAttribute; // "fluid", "vst" or "none"
  static const muse::String vstIdAttribute; // the VST's resource id
  static const muse::String fluidSynthValue;
  static const muse::String vstSynthValue;
  static const muse::String noSynthValue;

private:
  dgk::Inject<muse::vst::IVstInstancesRegister> vstInstancesRegister{this};

private:
  muse::audio::synth::ISynthesizerPtr
  resolveSynth(const muse::audio::TrackId,
               const muse::audio::AudioInputParams &,
               const muse::audio::OutputSpec &) const override;
  bool
  hasCompatibleResources(const muse::audio::PlaybackSetupData &) const override;
  muse::audio::AudioResourceMetaList resolveResources() const override;
  muse::audio::SoundPresetList
  resolveSoundPresets(const muse::audio::AudioResourceMeta &) const override;
  void refresh() override;
  void clearSources() override;
};
} // namespace dgk
