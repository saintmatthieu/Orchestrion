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

#include <chrono>
#include <string>

namespace dgk
{
/**
 * What the audio output needs to know while a testflow script's video is
 * rendered offline (--test-case-render, see App/OfflineRenderer), and what it
 * tells: OrchestrionSynthesis' OfflineAudioDriver writes the engine's output
 * to a file, on the main thread's clock.
 */
struct OfflineRendering
{
  /**
   * Where the audio goes, as interleaved 32-bit floats; empty when not
   * rendering.
   */
  std::string audioPath;

  /**
   * When the file's first sample was rendered, on the main thread's clock.
   */
  std::chrono::steady_clock::time_point audioStart;
  int sampleRate = 0;
  int channelCount = 0;
};

inline OfflineRendering offlineRendering;
} // namespace dgk
