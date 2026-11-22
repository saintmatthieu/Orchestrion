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
#include "RecordingSyncSession.h"

#include <algorithm>
#include <cstdlib>

namespace dgk
{
namespace
{
/** Anchors are this far apart at the least. */
constexpr double kResolution = 0.01;
/** Going back to tap a bar again: from this long before it. */
constexpr double kLeadIn = 2.0;
/** How many changes can be taken back. */
constexpr size_t kUndoDepth = 200;
} // namespace

RecordingSyncSession::RecordingSyncSession(std::vector<PlayedBar> bars,
                                           RecordingSync sync)
    : m_bars{std::move(bars)}, m_sync{std::move(sync)}
{
  if (m_sync.points.empty())
    m_next = m_bars.empty() ? std::nullopt : std::optional<int>{0};
  else
  {
    // On from the last anchor.
    m_last = Position(static_cast<int>(m_sync.points.size()) - 1);
    m_next = GridFrom(*m_last, false);
  }
}

int RecordingSyncSession::Position(int anchor) const
{
  const RecordingSync::Point &point = m_sync.points[anchor];
  return m_bars[point.bar].tick + point.tick;
}

int RecordingSyncSession::BarOf(int position) const
{
  const auto after = std::upper_bound(m_bars.begin(), m_bars.end(), position,
                                      [](int p, const PlayedBar &bar)
                                      { return p < bar.tick; });
  return std::max(0, static_cast<int>(after - m_bars.begin()) - 1);
}

RecordingSyncSession::Place RecordingSyncSession::PlaceOf(int position) const
{
  Place place;
  place.bar = BarOf(position);
  const PlayedBar &bar = m_bars[place.bar];
  const int tick = position - bar.tick;
  place.beat = tick / bar.beatTicks + 1;
  place.subdivisions = bar.beatTicks / std::max(1, bar.subdivisionTicks);
  place.subdivision =
      tick % bar.beatTicks / std::max(1, bar.subdivisionTicks) + 1;
  return place;
}

int RecordingSyncSession::Step(int bar) const
{
  return m_precision == Precision::beats ? m_bars[bar].beatTicks
                                         : m_bars[bar].subdivisionTicks;
}

std::optional<int> RecordingSyncSession::GridFrom(int position, bool at) const
{
  const int index = BarOf(position);
  const PlayedBar &bar = m_bars[index];
  const int step = std::max(1, Step(index));
  const int tick = position - bar.tick;
  const int grid =
      at ? (tick + step - 1) / step * step : (tick / step + 1) * step;
  if (grid < bar.length)
    return bar.tick + grid;
  if (index + 1 < static_cast<int>(m_bars.size()))
    return m_bars[index + 1].tick;
  return std::nullopt; // the end
}

void RecordingSyncSession::SetPrecision(Precision precision)
{
  m_precision = precision;
  if (m_next)
    m_next = GridFrom(*m_next, true);
}

void RecordingSyncSession::PushUndo()
{
  m_undo.push_back({m_sync.points, m_next, m_last});
  if (m_undo.size() > kUndoDepth)
    m_undo.erase(m_undo.begin());
  m_moving = -1;
}

bool RecordingSyncSession::Tap(double seconds)
{
  if (!m_next)
    return false;
  PushUndo();
  const int at = *m_next;
  // What the tap replaces: the anchors since the tap before, and any that
  // the time tapped puts out of order.
  const int from = m_last && *m_last < at ? *m_last : at - 1;
  std::vector<RecordingSync::Point> points;
  points.reserve(m_sync.points.size() + 1);
  for (int i = 0; i < static_cast<int>(m_sync.points.size()); ++i)
  {
    const int position = Position(i);
    const double s = m_sync.points[i].seconds;
    const bool replaced = (position > from && position <= at) ||
                          (position > at && s <= seconds) ||
                          (position < at && s >= seconds);
    if (!replaced)
      points.push_back(m_sync.points[i]);
  }
  const int bar = BarOf(at);
  const RecordingSync::Point tapped{bar, at - m_bars[bar].tick, seconds};
  const auto place = std::find_if(
      points.begin(), points.end(), [&](const RecordingSync::Point &point)
      { return m_bars[point.bar].tick + point.tick > at; });
  points.insert(place, tapped);
  m_sync.points = std::move(points);
  m_selection.clear();
  m_last = at;
  m_next = GridFrom(at, false);
  return true;
}

std::optional<double> RecordingSyncSession::TapFrom(int bar)
{
  if (bar < 0 || bar >= static_cast<int>(m_bars.size()))
    return std::nullopt;
  const int start = m_bars[bar].tick;
  m_next = start;
  m_last.reset();
  // Anchored there already (some anchor at its start or later): back to a
  // moment before the last anchor up to its start, else the first after.
  bool anchored = false;
  std::optional<double> upToStart, afterStart;
  for (int i = 0; i < static_cast<int>(m_sync.points.size()); ++i)
  {
    const int position = Position(i);
    anchored |= position >= start;
    if (position <= start)
      upToStart = m_sync.points[i].seconds;
    else if (!afterStart)
      afterStart = m_sync.points[i].seconds;
  }
  if (!anchored)
    return std::nullopt;
  return std::max(0.0, upToStart.value_or(*afterStart) - kLeadIn);
}

std::optional<int> RecordingSyncSession::BarAt(int writtenTick, int near) const
{
  std::optional<int> found;
  for (int i = 0; i < static_cast<int>(m_bars.size()); ++i)
  {
    const PlayedBar &bar = m_bars[i];
    if (writtenTick < bar.writtenTick ||
        writtenTick >= bar.writtenTick + bar.length)
      continue;
    if (!found ||
        std::abs(bar.tick - near) < std::abs(m_bars[*found].tick - near))
      found = i;
  }
  return found;
}

int RecordingSyncSession::AnchorAt(double seconds) const
{
  const auto after =
      std::upper_bound(m_sync.points.begin(), m_sync.points.end(), seconds,
                       [](double s, const RecordingSync::Point &point)
                       { return s < point.seconds; });
  return static_cast<int>(after - m_sync.points.begin()) - 1;
}

double RecordingSyncSession::Minimum(int anchor) const
{
  return anchor > 0 ? m_sync.points[anchor - 1].seconds + kResolution : 0.0;
}

double RecordingSyncSession::Maximum(int anchor) const
{
  return anchor + 1 < static_cast<int>(m_sync.points.size())
             ? m_sync.points[anchor + 1].seconds - kResolution
             : m_sync.points[anchor].seconds + 3600.0;
}

void RecordingSyncSession::SetSeconds(int anchor, double seconds)
{
  if (anchor < 0 || anchor >= static_cast<int>(m_sync.points.size()))
    return;
  if (anchor != m_moving)
  {
    PushUndo();
    m_moving = anchor;
  }
  m_sync.points[anchor].seconds =
      std::clamp(seconds, Minimum(anchor), Maximum(anchor));
}

void RecordingSyncSession::Select(int anchor, bool add)
{
  if (anchor < 0 || anchor >= static_cast<int>(m_sync.points.size()))
    return;
  if (!add)
    m_selection = {anchor};
  else if (!m_selection.erase(anchor))
    m_selection.insert(anchor);
}

void RecordingSyncSession::ExtendSelection(int direction, int from)
{
  const int count = static_cast<int>(m_sync.points.size());
  if (m_selection.empty())
  {
    if (from >= 0 && from < count)
      m_selection.insert(from);
    return;
  }
  const int added =
      direction > 0 ? *m_selection.rbegin() + 1 : *m_selection.begin() - 1;
  if (added >= 0 && added < count)
    m_selection.insert(added);
}

bool RecordingSyncSession::ClearSelection()
{
  if (m_selection.empty())
    return false;
  m_selection.clear();
  return true;
}

bool RecordingSyncSession::DeleteSelection()
{
  if (m_selection.empty())
    return false;
  PushUndo();
  // From the last, the indices before it staying.
  for (auto it = m_selection.rbegin(); it != m_selection.rend(); ++it)
    m_sync.points.erase(m_sync.points.begin() + *it);
  m_selection.clear();
  return true;
}

bool RecordingSyncSession::Undo()
{
  if (m_undo.empty())
    return false;
  State &state = m_undo.back();
  m_sync.points = std::move(state.points);
  m_next = state.next;
  m_last = state.last;
  m_undo.pop_back();
  m_selection.clear();
  m_moving = -1;
  return true;
}
} // namespace dgk
