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
#include <optional>

namespace dgk
{
/**
 * Where a video is, from what its player tells now and then. The time a
 * player tells may be old (YouTube's embedded player tells the one its page
 * last heard of, a few times a second); never ahead. So, while it plays at a
 * rate, its time less the rate times the clock's is taken as the largest
 * told since it started, or was moved: old times don't pull it back.
 */
class VideoClock
{
public:
  using TimePoint = std::chrono::steady_clock::time_point;

  /**
   * The player tells: `seconds` into the video at `when`, going at `rate`
   * if `playing`. Returns how old the time told was, in seconds, by what was
   * told before; 0 if it starts afresh (playing, moved, the rate changed).
   */
  double Tell(double seconds, TimePoint when, double rate, bool playing);

  /** Where the video is at `when`. */
  double SecondsAt(TimePoint when) const;
  bool Playing() const { return m_offset.has_value(); }
  double Rate() const { return m_rate; }

private:
  double m_rate = 1.0;
  /** Paused, where. */
  double m_seconds = 0.0;
  /** Playing, its time less the rate times the clock's. */
  std::optional<double> m_offset;
};
} // namespace dgk
