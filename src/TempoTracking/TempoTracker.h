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

  //! Advance the model's notion of "now" without a real onset (call this each
  //! frame). Once \p realTime overruns the expected time of the next onset (the
  //! recent inter-onset interval past the last one), the performer is late, so
  //! positionAt() switches from linear extrapolation to a decaying-velocity
  //! coast: it eases to a monotonic stop (no overshoot, no reversing, no reset)
  //! instead of running away. A real addObservation() ends the coast. No effect
  //! before warm-up.
  void heartbeat(double realTime);

  //! Whether the next onset is overdue and the extrapolation is coasting to
  //! rest (rather than tracking live).
  bool isCoasting() const { return _coasting; }

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
  //! Fitted/coasting position before the continuity offset is applied.
  double baseAt(double realTime) const;
  //! The decaying offset that keeps positionAt() continuous across refits.
  double offsetAt(double realTime) const;

  const int _maxObservations;
  const int _minObservations;
  // (realTime, musicalPos), oldest first.
  std::deque<std::pair<double, double>> _observations;
  double _slope = 0.0;     // musical units per real-time unit
  double _intercept = 0.0; // musical position at realTime == _refTime
  double _refTime = 0.0;   // realTime offset, for numerical conditioning
  bool _ready = false;

  // Decaying-velocity coast while the next onset is overdue (set by heartbeat,
  // ended by addObservation). Captured once when the coast begins:
  bool _coasting = false;
  double _coastT0 = 0.0;  // realTime the coast began
  double _coastP0 = 0.0;  // position at that time
  double _coastV0 = 0.0;  // velocity (tempo) at that time
  double _coastTau = 0.0; // decay time constant (∝ inter-onset interval)

  // Continuity offset: set to the jump a refit would cause, then decayed to 0,
  // so positionAt() slides to a new fit gradually instead of snapping.
  double _displayOffset0 = 0.0;
  double _offsetT0 = 0.0;
};
} // namespace dgk
