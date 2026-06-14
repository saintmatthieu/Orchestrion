/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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

#include <deque>
#include <utility>

namespace dgk
{
//! Estimates a performer's musical position over time from sparse "where they
//! are now" observations, so playback (a scrolling view, an auto-accompaniment,
//! a rhythm grader) can advance smoothly *between* observations.
//!
//! It keeps a sliding window of recent (realTime, musicalPos) observations and
//! fits a straight line musicalPos ≈ speed·realTime + phase by weighted least
//! squares, weighting recent observations more (a raised-cosine window). The
//! fitted line extrapolates the current position: when the performer holds a
//! constant tempo the observations are collinear, so the position advances at a
//! constant speed — no jumps.
//!
//! Units are caller-defined: realTime is any monotonic clock (ms, seconds, …)
//! and musicalPos any linear musical coordinate (score ticks, beats, …). The
//! tracker is deliberately model-agnostic and dependency-free so it can back
//! several features.
//!
//! The algorithm is ported from the tempo-driven controller of the Christophone
//! prototype (weighted least squares + raised-cosine recency weighting + a
//! fixed-size history), reimplemented here with plain 2×2 normal equations
//! (no Eigen) in double precision.
class TempoTracker
{
public:
  //! \p maxObservations is the sliding-window size (older observations are
  //! dropped). \p minObservations is the warm-up gate: no estimate is produced
  //! until at least this many observations have accumulated. Observations are
  //! expected roughly one per beat/onset, strictly increasing in realTime.
  explicit TempoTracker(int maxObservations = 10, int minObservations = 3);

  //! Record that at \p realTime the performance reached musical position
  //! \p musicalPos, and refit.
  void addObservation(double realTime, double musicalPos);

  //! Whether enough observations have accumulated to produce an estimate.
  bool ready() const { return _ready; }

  //! Extrapolated musical position at \p realTime. Before warm-up, returns the
  //! most recent observed position (or 0 if there is none).
  double positionAt(double realTime) const;

  //! Estimated tempo: musical units per unit of real time (the fitted slope).
  //! 0 before warm-up.
  double speed() const { return _ready ? _slope : 0.0; }

  //! Timing error for rhythm evaluation: how far an observation of
  //! \p musicalPos at \p realTime lies ahead (+) or behind (−) the current
  //! fitted trajectory, in musical units.
  double predictionError(double realTime, double musicalPos) const
  {
    return musicalPos - positionAt(realTime);
  }

  //! Forget all history (e.g. a position jump or a new piece).
  void reset();

private:
  void refit();

  const int _maxObservations;
  const int _minObservations;
  // (realTime, musicalPos), oldest first.
  std::deque<std::pair<double, double>> _observations;
  double _slope = 0.0;     // musical units per real-time unit
  double _intercept = 0.0; // musical position at realTime == _refTime
  double _refTime = 0.0;   // realTime offset, for numerical conditioning
  bool _ready = false;
};
} // namespace dgk
