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

#include "PerformanceFollower.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace dgk
{
/** A note of a recorded performance, as transcribed. */
struct PerformedNote
{
  double onsetMs = 0.0;
  double offsetMs = 0.0;
  int pitch = 0;
  int velocity = 0;
};

/** Where in a performance each event of the score was played. */
struct PerformanceAlignment
{
  static constexpr size_t none = static_cast<size_t>(-1);

  struct Event
  {
    /**
     * When it was played (the median onset of its notes found, the earlier of
     * the middle two), if it was.
     */
    std::optional<double> ms;
    /**
     * For each of the event's pitches (as ScoreEvent::pitches lists them),
     * the performed note that played it, or `none`.
     */
    std::vector<size_t> notes;
    /**
     * The other notes of a trill beginning here, in time order: as many as
     * the performer played.
     */
    std::vector<size_t> trillNotes;
  };

  /** One per score event. */
  std::vector<Event> events;
  /**
   * The performed notes the score has no place for, in time order: the
   * performer's additions, or the transcription's mistakes.
   */
  std::vector<size_t> added;
};

/**
 * Aligns a whole performance to the score, offline. `events` are as for
 * PerformanceFollower, with repeats; `notes` are sorted by onset. A performer
 * may skip a repeat. Performed notes are given by their index in `notes`.
 */
PerformanceAlignment AlignPerformance(const std::vector<ScoreEvent> &events,
                                      const std::vector<PerformedNote> &notes,
                                      double nominalTicksPerMs);
} // namespace dgk
