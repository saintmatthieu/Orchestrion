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
#include "RecordingTimeline.h"

#include <algorithm>

namespace dgk
{
namespace
{
/** A last beat whose pace nothing tells: about a slow beat's. */
constexpr double kDefaultBeatSeconds = 0.6;
} // namespace

std::vector<RecordingBeat> SmoothBeats(std::vector<RecordingBeat> beats,
                                       int passes)
{
  for (int pass = 0; pass < passes; ++pass)
  {
    std::vector<double> seconds(beats.size());
    for (size_t i = 0; i < beats.size(); ++i)
    {
      seconds[i] = beats[i].seconds;
      // Between two beats it is played on from and to.
      if (i == 0 || i + 1 == beats.size() || beats[i].afterSkip ||
          beats[i + 1].afterSkip)
        continue;
      const RecordingBeat &before = beats[i - 1];
      const RecordingBeat &after = beats[i + 1];
      const double between =
          before.seconds + (after.seconds - before.seconds) *
                               (beats[i].tick - before.tick) /
                               (after.tick - before.tick);
      seconds[i] = (beats[i].seconds + 2.0 * between) / 3.0;
    }
    for (size_t i = 0; i < beats.size(); ++i)
      beats[i].seconds = seconds[i];
  }
  return beats;
}

RecordingTimeline::RecordingTimeline(std::vector<RecordingBeat> beats)
    : m_beats{std::move(beats)}
{
  for (size_t i = 1; i < m_beats.size(); ++i)
    if (m_beats[i].seconds <= m_beats[i - 1].seconds ||
        m_beats[i].tick <= m_beats[i - 1].tick)
    {
      m_beats.clear();
      return;
    }
}

int RecordingTimeline::ReachedTicks(size_t i) const
{
  if (i + 1 < m_beats.size() && !m_beats[i + 1].afterSkip)
    return m_beats[i + 1].tick - m_beats[i].tick;
  // Before a skip, or at the end: the beat itself.
  return m_beats[i].beatTicks;
}

namespace
{
/** The pace from beat `i` to the next, if they are played one after the other.
 */
std::optional<double> Pace(const std::vector<RecordingBeat> &beats, size_t i)
{
  if (i + 1 >= beats.size() || beats[i + 1].afterSkip)
    return std::nullopt;
  return (beats[i + 1].seconds - beats[i].seconds) /
         (beats[i + 1].tick - beats[i].tick);
}

/** How long the music played from beat `i` lasts until the next beat. */
double SegmentSeconds(const std::vector<RecordingBeat> &beats, size_t i)
{
  if (i + 1 < beats.size())
    return beats[i + 1].seconds - beats[i].seconds;
  // The last: at the pace of the beat before, if it led to it.
  if (i > 0)
    if (const auto pace = Pace(beats, i - 1))
      return *pace * beats[i].beatTicks;
  return kDefaultBeatSeconds;
}
} // namespace

std::optional<RecordingTimeline::Position>
RecordingTimeline::PositionAt(double seconds) const
{
  const auto after = std::upper_bound(m_beats.begin(), m_beats.end(), seconds,
                                      [](double s, const RecordingBeat &beat)
                                      { return s < beat.seconds; });
  if (after == m_beats.begin())
    return std::nullopt;
  const auto i = static_cast<size_t>(after - m_beats.begin() - 1);
  const RecordingBeat &beat = m_beats[i];
  const double duration = SegmentSeconds(m_beats, i);
  const double fraction =
      duration > 0.0 ? std::min((seconds - beat.seconds) / duration, 1.0) : 0.0;
  const int ticks = static_cast<int>(fraction * ReachedTicks(i));
  return Position{beat.tick + ticks, beat.writtenTick + ticks};
}

std::optional<double> RecordingTimeline::SecondsAt(int tick) const
{
  const auto after = std::upper_bound(m_beats.begin(), m_beats.end(), tick,
                                      [](int t, const RecordingBeat &beat)
                                      { return t < beat.tick; });

  // Within a beat before one the recording starts or goes on at (grace
  // notes, due before the beat): back from it, at its pace.
  if (after != m_beats.end() &&
      (after == m_beats.begin() || after->afterSkip) &&
      after->tick - tick <= after->beatTicks)
  {
    const auto i = static_cast<size_t>(after - m_beats.begin());
    const double pace =
        Pace(m_beats, i).value_or(kDefaultBeatSeconds / after->beatTicks);
    return after->seconds - (after->tick - tick) * pace;
  }
  if (after == m_beats.begin())
    return std::nullopt;

  const auto i = static_cast<size_t>(after - m_beats.begin() - 1);
  const RecordingBeat &beat = m_beats[i];
  const int reached = ReachedTicks(i);
  if (tick - beat.tick > reached)
    return std::nullopt; // skipped, or past the end
  return beat.seconds +
         SegmentSeconds(m_beats, i) * (tick - beat.tick) / reached;
}

std::optional<double> RecordingTimeline::SecondsPerTick(int tick) const
{
  const auto after = std::upper_bound(m_beats.begin(), m_beats.end(), tick,
                                      [](int t, const RecordingBeat &beat)
                                      { return t < beat.tick; });
  const auto at = static_cast<size_t>(
      std::max<std::ptrdiff_t>(0, after - m_beats.begin() - 1));
  // Outwards from there, before first, the first two beats played one
  // after the other.
  for (size_t distance = 0; distance < m_beats.size(); ++distance)
    for (const auto i : {at - distance, at + distance})
      if (i < m_beats.size())
        if (const auto pace = Pace(m_beats, i))
          return pace;
  return std::nullopt;
}

const RecordingBeat *RecordingTimeline::NextPlayed(int tick) const
{
  const auto it = std::lower_bound(m_beats.begin(), m_beats.end(), tick,
                                   [](const RecordingBeat &beat, int t)
                                   { return beat.tick < t; });
  return it == m_beats.end() ? nullptr : &*it;
}
} // namespace dgk
