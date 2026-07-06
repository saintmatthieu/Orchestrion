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

namespace dgk
{
//! Estimates a performer's musical position and tempo over time from sparse
//! onset observations, so playback (a scrolling view, an auto-accompaniment, a
//! rhythm grader) can advance smoothly *between* onsets.
//!
//! It is an α–β filter — the steady-state Kalman filter for a constant-velocity
//! model, i.e. the simplest real-time form of fitting a smooth (spline-like)
//! position curve. State is (position, velocity=tempo). Between onsets the
//! state is propagated forward each frame via heartbeat() (a constant-velocity
//! prediction); each onset nudges position and tempo toward the observation by
//! the gains α and β. When the next onset is overdue the velocity is damped so
//! the view coasts to a stop instead of running away.
//!
//! There is essentially one tuning knob: \p memory (γ ∈ (0,1)). Higher =
//! smoother / longer memory (α = 1−γ², β = (1−γ)² are derived from it,
//! critically damped, so there are no separate process/measurement variances to
//! set). The coast deceleration has its own time constant (see the .cpp).
//!
//! Units are caller-defined: realTime is any monotonic clock (ms) and
//! musicalPos any linear musical coordinate (score ticks, page-logical x, …).
//! The estimator is model-agnostic and dependency-free so it can back several
//! features. The α/β recency idea is the real-time analogue of the weighted fit
//! in the Christophone tempo-driven controller it was ported from.
class TempoTracker
{
public:
  //! \p memory is γ ∈ (0,1): higher tracks more smoothly (longer memory).
  explicit TempoTracker(double memory = 0.6);

  //! Record that at \p realTime the performance reached musical position
  //! \p musicalPos: predict to that time, then correct the state.
  void addObservation(double realTime, double musicalPos);

  //! Advance the state to \p realTime without an onset (call each frame).
  //! Glides at the current tempo; once the next onset is overdue, damps the
  //! tempo so the view coasts to a stop rather than extrapolating off the end.
  void heartbeat(double realTime);

  //! Whether the next onset is overdue and the view is coasting to rest.
  bool isCoasting() const { return _coasting; }

  //! Whether a tempo estimate is available yet (needs two onsets for velocity).
  bool ready() const { return _nObs >= 2; }

  //! Estimated musical position at \p realTime (constant-velocity prediction
  //! from the last state). Before warm-up, the last observed position (or 0).
  double positionAt(double realTime) const;

  //! Estimated tempo: musical units per unit of real time. 0 before warm-up.
  double speed() const { return ready() ? _v : 0.0; }

  //! Smoothed inter-onset interval (ms), the playing cadence. 0 until known.
  double intervalMs() const { return _intervalMs; }

  //! Number of onsets observed since construction or reset() — lets callers
  //! gate on the estimate having settled past its two-onset seeding.
  int observations() const { return _nObs; }

  //! Timing error for rhythm evaluation: how far an observation of
  //! \p musicalPos at \p realTime lies ahead (+) or behind (−) the prediction.
  double predictionError(double realTime, double musicalPos) const
  {
    return musicalPos - positionAt(realTime);
  }

  //! Forget all history (e.g. a position jump or a new piece).
  void reset();

private:
  //! Constant-velocity prediction of the state up to \p realTime (no damping).
  void advanceTo(double realTime);
  //! Filter prediction before the continuity offset is applied.
  double baseAt(double realTime) const;
  //! The decaying offset that keeps positionAt() continuous across corrections.
  double offsetAt(double realTime) const;

  const double _alpha; // position gain
  const double _beta;  // velocity gain

  double _p = 0.0;          // estimated musical position
  double _v = 0.0;          // estimated tempo (musical units / ms)
  double _lastTime = 0.0;   // realTime the state was last propagated to
  double _lastObsTime = -1; // realTime of the last onset (-1 = none)
  double _intervalMs = 0.0; // smoothed inter-onset interval (0 = unknown)
  int _nObs = 0;
  bool _coasting = false;

  // Continuity offset: set to the jump a correction would cause, decayed to 0,
  // so positionAt() slides to a corrected position gradually instead of
  // snapping.
  double _displayOffset0 = 0.0;
  double _offsetT0 = 0.0;
};
} // namespace dgk
