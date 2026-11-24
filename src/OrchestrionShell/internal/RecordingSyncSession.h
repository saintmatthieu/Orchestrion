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

#include "RecordingSync.h"

#include <optional>
#include <set>
#include <vector>

namespace dgk
{
/**
 * Syncing a recording with a score: its anchors (the points of its sync,
 * each a beat of the score and when the recording plays it), as they are
 * tapped, rewritten and deleted.
 *
 * Taps go through the score's beats in order (a pickup's as its bar's last
 * ones), from where the last left off or from a bar chosen, up to its last
 * chord. Each anchors the beat it is for, and replaces the anchors between
 * it and the tap before: going over a passage again rewrites it. Anchors
 * stay in the order played, in time as in the score.
 *
 * Positions are in ticks with repeats (as played); seconds are the piece's,
 * from its start in the recording.
 */
class RecordingSyncSession
{
public:
  /** Where a position is: in which bar, and beat (from 1). */
  struct Place
  {
    int bar = 0;
    int beat = 1;
  };

  /** `sync` must fit `bars` (see ReadRecordingSync). */
  RecordingSyncSession(std::vector<PlayedBar> bars, RecordingSync sync);

  const std::vector<PlayedBar> &Bars() const { return m_bars; }
  const RecordingSync &Sync() const { return m_sync; }
  /** Where an anchor is, in ticks with repeats. */
  int Position(int anchor) const;
  Place PlaceOf(int position) const;

  /** The position the next tap is for; none past the score's end. */
  std::optional<int> Next() const { return m_next; }
  /**
   * A tap, `seconds` into the piece. False if it was past the score's end,
   * or before the piece's start.
   */
  bool Tap(double seconds);
  /**
   * The next tap is for the start of `bar`. If the recording was anchored
   * there already, where to go back to, a moment before, to tap it again.
   */
  std::optional<double> TapFrom(int bar);
  /**
   * The bar as played whose part of the score as written `writtenTick` is
   * in: of a repeat's passes, the nearest `near` (a position); but if the
   * next tap is already for the start of one of them (clicked again), the
   * pass after it, the last's being the first.
   */
  std::optional<int> BarAt(int writtenTick, int near) const;

  /** The anchor the recording is at `seconds`: the last before; -1 if none. */
  int AnchorAt(double seconds) const;

  const std::set<int> &Selection() const { return m_selection; }
  /** Selects the anchor; added to (or taken from) the selection if `add`. */
  void Select(int anchor, bool add);
  /**
   * Adds the anchor after the selection (`direction` > 0) or before it; if
   * none is selected, `from`.
   */
  void ExtendSelection(int direction, int from);
  void SelectAll();
  /** False if nothing was selected. */
  bool ClearSelection();
  /**
   * False if nothing was selected. With all deleted, taps start again from
   * the score's start.
   */
  bool DeleteSelection();

  /** Takes the last change back (a tap, a deletion). */
  bool Undo();

private:
  struct State
  {
    std::vector<RecordingSync::Point> points;
    std::optional<int> next;
    std::optional<int> last;
  };

  int BarOf(int position) const;
  /**
   * The first beat after (or at, if `at`) a position; none
   * past the last chord struck: there is nothing to tap after it.
   */
  std::optional<int> GridFrom(int position, bool at) const;
  void PushUndo();

  const std::vector<PlayedBar> m_bars;
  /** The last chord struck in the score, as played; -1 if none. */
  int m_end = -1;
  RecordingSync m_sync;
  std::optional<int> m_next;
  /** The position tapped last, from which the next tap replaces anchors. */
  std::optional<int> m_last;
  std::set<int> m_selection;
  std::vector<State> m_undo;
};
} // namespace dgk
