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
#include "OrchestrionSynthResolver.h"
#include "AntiMetronomeSynthesizer.h"
#include "FluidSynthesizer.h"
#include "LowpassFilterBank.h"
#include "OrchestrionSynthesizerWrapper.h"
#include "OrchestrionVstSynthesizer.h"
#include "PromisedSynthesizer.h"

#include <log.h>
#include <optional>

namespace dgk
{
const muse::String OrchestrionSynthResolver::synthAttribute{u"orchestrionSynth"};
const muse::String OrchestrionSynthResolver::vstIdAttribute{u"orchestrionVstId"};
const muse::String OrchestrionSynthResolver::fluidSynthValue{u"fluid"};
const muse::String OrchestrionSynthResolver::vstSynthValue{u"vst"};
const muse::String OrchestrionSynthResolver::noSynthValue{u"none"};

muse::audio::synth::ISynthesizerPtr OrchestrionSynthResolver::resolveSynth(
    const muse::audio::TrackId trackId,
    const muse::audio::AudioInputParams &params,
    const muse::audio::OutputSpec &) const
{
  const muse::String synth = params.resourceMeta.attributeVal(synthAttribute);
  const bool fluid = synth == fluidSynthValue;
  std::optional<muse::audio::AudioResourceId> vstId;
  if (synth == vstSynthValue)
    vstId = params.resourceMeta.attributeVal(vstIdAttribute).toStdString();
  LOGI() << "Resolving Orchestrion synthesizer for track " << trackId << " ("
         << synth.toStdString() << ")";

  OrchestrionSynthesizerWrapper::SynthFactory factory =
      [this, trackId, vstId, fluid](const muse::audio::OutputSpec &spec)
      -> std::unique_ptr<IOrchestrionSynthesizer>
  {
    const int sampleRate = static_cast<int>(spec.sampleRate);
    if (vstId.has_value())
    {
      muse::async::Channel<std::shared_ptr<IOrchestrionSynthesizer>>
          synthLoaded;
      const muse::vst::IVstPluginInstancePtr instance =
          vstInstancesRegister()->makeAndRegisterInstrPlugin(*vstId, trackId);
      if (!instance)
        return nullptr;
      const auto makeSynth =
          [spec, sampleRate, trackId,
           instance]() -> std::unique_ptr<IOrchestrionSynthesizer>
      {
        return std::make_unique<AntiMetronomeSynthesizer>(
            sampleRate, trackId,
            [instance, spec](int)
            {
              return std::make_unique<OrchestrionVstSynthesizer>(instance,
                                                                 spec);
            });
      };
      if (instance->isLoaded())
        return makeSynth();
      instance->loadingCompleted().onNotify(
          this,
          [synthLoaded, makeSynth]() mutable
          {
            synthLoaded.send(
                std::shared_ptr<IOrchestrionSynthesizer>(makeSynth()));
          });
      return std::make_unique<PromisedSynthesizer>(synthLoaded);
    }
    else if (fluid)
      return std::make_unique<AntiMetronomeSynthesizer>(
          sampleRate, trackId,
          [](int sampleRate)
          {
            return std::make_unique<LowpassFilterBank>(
                [sampleRate]
                { return std::make_unique<FluidSynthesizer>(sampleRate); });
          });
    else
      return nullptr;
  };
  return std::make_shared<OrchestrionSynthesizerWrapper>(std::move(factory),
                                                         params);
}

bool OrchestrionSynthResolver::hasCompatibleResources(
    const muse::audio::PlaybackSetupData &) const
{
  return true;
}

muse::audio::AudioResourceMetaList
OrchestrionSynthResolver::resolveResources() const
{
  return {};
}

muse::audio::SoundPresetList OrchestrionSynthResolver::resolveSoundPresets(
    const muse::audio::AudioResourceMeta &) const
{
  return {};
}

void OrchestrionSynthResolver::refresh() {}

void OrchestrionSynthResolver::clearSources() {}
} // namespace dgk
