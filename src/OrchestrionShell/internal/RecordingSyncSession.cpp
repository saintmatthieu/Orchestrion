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
/** Going back to tap a bar again: from this long before it. */
constexpr double kLeadIn = 2.0;
/** How many changes can be taken back. */
constexpr size_t kUndoDepth = 200;
} // namespace

RecordingSyncSession::RecordingSyncSession(std::vector<PlayedBar> bars,
                                           RecordingSync sync)
    : m_bars{std::move(bars)}, m_sync{std::move(sync)}
{
  for (auto bar = m_bars.rbegin(); bar != m_bars.rend() && m_end < 0; ++bar)
    if (bar->lastOnset >= 0)
      m_end = bar->tick + bar->lastOnset;
  if (m_sync.points.empty())
    m_next = m_end < 0 ? std::nullopt : GridFrom(0, true);
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
  const int tick = position - bar.tick + bar.gridOffset;
  place.beat = tick / bar.beatTicks + 1;
  return place;
}

std::optional<int> RecordingSyncSession::GridFrom(int position, bool at) const
{
  const int index = BarOf(position);
  const PlayedBar &bar = m_bars[index];
  const int step = std::max(1, bar.beatTicks);
  // Its time's beats: a pickup's from where its whole bar would start.
  const int tick = position - bar.tick + bar.gridOffset;
  const int grid =
      at ? (tick + step - 1) / step * step : (tick / step + 1) * step;
  std::optional<int> found;
  if (grid - bar.gridOffset < bar.length)
    found = bar.tick + grid - bar.gridOffset;
  else if (index + 1 < static_cast<int>(m_bars.size()))
    found = m_bars[index + 1].tick;
  if (!found || *found > m_end)
    return std::nullopt; // nothing to tap after the last chord
  return found;
}

void RecordingSyncSession::PushUndo()
{
  m_undo.push_back({m_sync.points, m_next, m_last});
  if (m_undo.size() > kUndoDepth)
    m_undo.erase(m_undo.begin());
}

bool RecordingSyncSession::Tap(double seconds)
{
  // Before the piece starts in the recording, it isn't being played.
  if (!m_next || seconds < 0.0)
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
  // Its passes, in the order played.
  std::vector<int> passes;
  for (int i = 0; i < static_cast<int>(m_bars.size()); ++i)
    if (writtenTick >= m_bars[i].writtenTick &&
        writtenTick < m_bars[i].writtenTick + m_bars[i].length)
      passes.push_back(i);
  if (passes.empty())
    return std::nullopt;
  for (size_t p = 0; p < passes.size(); ++p)
    if (m_next == m_bars[passes[p]].tick)
      return passes[(p + 1) % passes.size()];
  return *std::min_element(passes.begin(), passes.end(),
                           [&](int a, int b)
                           {
                             return std::abs(m_bars[a].tick - near) <
                                    std::abs(m_bars[b].tick - near);
                           });
}

int RecordingSyncSession::AnchorAt(double seconds) const
{
  const auto after =
      std::upper_bound(m_sync.points.begin(), m_sync.points.end(), seconds,
                       [](double s, const RecordingSync::Point &point)
                       { return s < point.seconds; });
  return static_cast<int>(after - m_sync.points.begin()) - 1;
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

void RecordingSyncSession::SelectAll()
{
  for (int i = 0; i < static_cast<int>(m_sync.points.size()); ++i)
    m_selection.insert(i);
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
  if (m_sync.points.empty())
  {
    m_last.reset();
    m_next = m_end < 0 ? std::nullopt : GridFrom(0, true);
  }
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
  return true;
}
} // namespace dgk
