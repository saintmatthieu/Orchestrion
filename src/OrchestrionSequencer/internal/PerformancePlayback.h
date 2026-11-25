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

#include "Performance.h"
#include "PerformanceAligner.h"
#include "RecordingTimeline.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace dgk
{
/** How loud each note of the score was performed, from 0 to 1. */
class PerformedVelocities
{
public:
  /** The note of `pitch` struck at `tick` (with repeats), if performed. */
  std::optional<float> At(int tick, int pitch) const;
  void Set(int tick, int pitch, float velocity);
  bool Empty() const { return m_velocities.empty(); }

private:
  std::unordered_map<std::int64_t, float> m_velocities;
};

/** What the score is played with to play it as a performance was. */
struct PerformancePlayback
{
  /**
   * When each event was performed, unsmoothed; null if none was. A repeat
   * the performer skipped is a skip.
   */
  std::shared_ptr<const RecordingTimeline> timeline;
  std::shared_ptr<const PerformedVelocities> velocities;
};

/** From a performance and its alignment to `events` (see AlignPerformance). */
PerformancePlayback
MakePerformancePlayback(const std::vector<ScoreEvent> &events,
                        const std::vector<PerformedNote> &notes,
                        const PerformanceAlignment &alignment);
} // namespace dgk
