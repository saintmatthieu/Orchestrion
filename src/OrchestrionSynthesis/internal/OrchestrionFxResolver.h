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
#pragma once

#include "BuiltInEffectRuntime.h"
#include <engine/internal/fx/abstractfxresolver.h>

namespace dgk
{
/**
 * Resolves Orchestrion's built-in effects for the audio engine, in place of
 * MuseScore's native-fx resolver (Muse Reverb is one of them, wrapped so that
 * its parameters can be driven).
 */
class OrchestrionFxResolver : public muse::audio::fx::AbstractFxResolver
{
public:
  explicit OrchestrionFxResolver(BuiltInEffectRuntime runtime);

  muse::audio::AudioResourceMetaList resolveResources() const override;

private:
  muse::audio::IFxProcessorPtr
  createMasterFx(const muse::audio::AudioFxParams &fxParams,
                 const muse::audio::OutputSpec &outputSpec) const override;
  muse::audio::IFxProcessorPtr
  createTrackFx(muse::audio::TrackId trackId,
                const muse::audio::AudioFxParams &fxParams,
                const muse::audio::OutputSpec &outputSpec) const override;

  muse::audio::IFxProcessorPtr
  createFx(const muse::audio::AudioFxParams &fxParams,
           const muse::audio::OutputSpec &outputSpec) const;

  const BuiltInEffectRuntime m_runtime;
};
} // namespace dgk
