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

#include <atomic>

namespace dgk
{
/**
 * Orchestrion's built-in effects: a compressor and a limiter (the same
 * processor, Daniel Rudrich's SimpleCompressor driven by Audacity's
 * CompressorProcessor, with two sets of parameters) and MuseScore's reverb.
 */
enum class BuiltInEffect
{
  Compressor,
  Limiter,
  Reverb,
};

/**
 * The reverb's presets: the spaces a pianist knows, from the living room to
 * the nave. Tuned by ear on the piano; the parameters behind them are in
 * internal/ReverbPresetParameters.h.
 */
enum class ReverbPreset
{
  Room,
  SmallHall,
  LargeHall,
  Cathedral,
};

/**
 * What a built-in effect shows of itself: written by the audio thread after
 * every block, read by the UI at its own pace. Levels are linear peaks (1 is
 * full scale); the clip flag stays up until the UI resets it.
 */
struct EffectMeter
{
  std::atomic<float> inputPeak{0.f};
  std::atomic<float> outputPeak{0.f};
  std::atomic<float> gainReductionDb{0.f};
  std::atomic<bool> clipped{false};
};
} // namespace dgk
