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
#include "VideoClock.h"

#include <cmath>

namespace dgk
{
namespace
{
/**
 * A time told this far from the estimate is the video moved (a seek), not
 * an old time: older than this, none is.
 */
constexpr double kMoved = 0.75;

double Seconds(VideoClock::TimePoint when)
{
  return std::chrono::duration<double>(when.time_since_epoch()).count();
}
} // namespace

double VideoClock::Tell(double seconds, TimePoint when, double rate,
                        bool playing)
{
  if (!playing)
  {
    m_offset.reset();
    m_seconds = seconds;
    m_rate = rate;
    return 0.0;
  }
  const double offset = seconds - rate * Seconds(when);
  if (!m_offset || rate != m_rate || std::abs(offset - *m_offset) > kMoved)
  {
    m_offset = offset;
    m_rate = rate;
    return 0.0;
  }
  const double old = std::max(0.0, *m_offset - offset);
  m_offset = std::max(*m_offset, offset);
  return old;
}

double VideoClock::SecondsAt(TimePoint when) const
{
  return m_offset ? *m_offset + m_rate * Seconds(when) : m_seconds;
}
} // namespace dgk
