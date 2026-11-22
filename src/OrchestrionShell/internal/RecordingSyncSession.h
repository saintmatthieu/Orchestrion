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
 * each a position in the score and when the recording plays it), as they
 * are tapped, rewritten, moved and deleted.
 *
 * Taps go through the score in order, at the precision chosen (beats, or
 * their subdivisions), from where the last left off or from a bar chosen.
 * Each anchors the position it is for, and replaces the anchors between it
 * and the tap before: going over a passage again rewrites it, finer or
 * coarser. Anchors stay in the order played, in time as in the score.
 *
 * Positions are in ticks with repeats (as played); seconds are the piece's,
 * from its start in the recording.
 */
class RecordingSyncSession
{
public:
  enum class Precision
  {
    beats,
    subdivisions,
  };

  /** Where a position is: in which bar, beat and subdivision (from 1). */
  struct Place
  {
    int bar = 0;
    int beat = 1;
    int subdivision = 1;
    /** How many subdivisions its beat has. */
    int subdivisions = 1;
  };

  /** `sync` must fit `bars` (see ReadRecordingSync). */
  RecordingSyncSession(std::vector<PlayedBar> bars, RecordingSync sync);

  const std::vector<PlayedBar> &Bars() const { return m_bars; }
  const RecordingSync &Sync() const { return m_sync; }
  /** Where an anchor is, in ticks with repeats. */
  int Position(int anchor) const;
  Place PlaceOf(int position) const;

  Precision GetPrecision() const { return m_precision; }
  /** The next tap is for the first position at this precision from here. */
  void SetPrecision(Precision);

  /** The position the next tap is for; none past the score's end. */
  std::optional<int> Next() const { return m_next; }
  /** A tap, `seconds` into the piece. False if it was past the end. */
  bool Tap(double seconds);
  /**
   * The next tap is for the start of `bar`. If the recording was anchored
   * there already, where to go back to, a moment before, to tap it again.
   */
  std::optional<double> TapFrom(int bar);
  /**
   * The bar as played whose part of the score as written `writtenTick` is
   * in: of a repeat's passes, the nearest `near` (a position).
   */
  std::optional<int> BarAt(int writtenTick, int near) const;

  /** The anchor the recording is at `seconds`: the last before; -1 if none. */
  int AnchorAt(double seconds) const;
  /** How early and late an anchor may be: just after and before its neighbours.
   */
  double Minimum(int anchor) const;
  double Maximum(int anchor) const;
  /** Moves an anchor, kept between its neighbours. */
  void SetSeconds(int anchor, double seconds);

  const std::set<int> &Selection() const { return m_selection; }
  /** Selects the anchor; added to (or taken from) the selection if `add`. */
  void Select(int anchor, bool add);
  /**
   * Adds the anchor after the selection (`direction` > 0) or before it; if
   * none is selected, `from`.
   */
  void ExtendSelection(int direction, int from);
  /** False if nothing was selected. */
  bool ClearSelection();
  /** False if nothing was selected. */
  bool DeleteSelection();

  /** Takes the last change back (a tap, a deletion, an anchor moved). */
  bool Undo();

private:
  struct State
  {
    std::vector<RecordingSync::Point> points;
    std::optional<int> next;
    std::optional<int> last;
  };

  int BarOf(int position) const;
  int Step(int bar) const;
  /** The first position at this precision after (or at, if `at`) one. */
  std::optional<int> GridFrom(int position, bool at) const;
  void PushUndo();

  const std::vector<PlayedBar> m_bars;
  RecordingSync m_sync;
  Precision m_precision = Precision::beats;
  std::optional<int> m_next;
  /** The position tapped last, from which the next tap replaces anchors. */
  std::optional<int> m_last;
  std::set<int> m_selection;
  std::vector<State> m_undo;
  /** The anchor moved last: moving it on is one change, for undoing. */
  int m_moving = -1;
};
} // namespace dgk
