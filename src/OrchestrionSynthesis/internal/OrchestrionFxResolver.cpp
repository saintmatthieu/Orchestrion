/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "OrchestrionFxResolver.h"
#include "BuiltInEffectResources.h"
#include "DynamicsProcessor.h"
#include <log.h>

namespace dgk
{
OrchestrionFxResolver::OrchestrionFxResolver(BuiltInEffectRuntime runtime)
    : m_runtime{std::move(runtime)}
{
}

muse::audio::AudioResourceMetaList
OrchestrionFxResolver::resolveResources() const
{
  muse::audio::AudioResourceMetaList result;
  for (const auto effect : allBuiltInEffects)
    result.push_back(builtInEffectMeta(effect));
  return result;
}

muse::audio::IFxProcessorPtr OrchestrionFxResolver::createMasterFx(
    const muse::audio::AudioFxParams &fxParams,
    const muse::audio::OutputSpec &outputSpec) const
{
  return createFx(fxParams, outputSpec);
}

muse::audio::IFxProcessorPtr OrchestrionFxResolver::createTrackFx(
    muse::audio::TrackId, const muse::audio::AudioFxParams &fxParams,
    const muse::audio::OutputSpec &outputSpec) const
{
  return createFx(fxParams, outputSpec);
}

muse::audio::IFxProcessorPtr
OrchestrionFxResolver::createFx(const muse::audio::AudioFxParams &fxParams,
                                const muse::audio::OutputSpec &outputSpec) const
{
  const std::optional<BuiltInEffect> effect =
      builtInEffectOf(fxParams.resourceMeta.id);
  if (!effect)
  {
    LOGD() << "Unknown native effect: " << fxParams.resourceMeta.id;
    return nullptr;
  }
  muse::audio::IFxProcessorPtr processor;
  switch (*effect)
  {
  case BuiltInEffect::Compressor:
    processor = std::make_shared<DynamicsProcessor>(
        *effect, fxParams, m_runtime.compressor, m_runtime.meters.at(*effect));
    break;
  case BuiltInEffect::Limiter:
    processor = std::make_shared<DynamicsProcessor>(
        *effect, fxParams, m_runtime.limiter, m_runtime.meters.at(*effect));
    break;
  }
  processor->setOutputSpec(outputSpec);
  return processor;
}
} // namespace dgk
