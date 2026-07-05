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
#include <vector>

namespace dgk
{
//! Companion to TempoTracker: fed the same sparse onsets, it maintains the
//! *smoothed* (a-posteriori) position curve over a recent window — the actual
//! smoothing spline the causal filter only chases the right endpoint of.
//!
//! Where TempoTracker answers "where is the performer now?" causally (its tempo
//! trace is a staircase: constant between onsets, nudged at each one), this
//! class re-fits the whole recent past on every onset: a Kalman forward pass
//! over the window followed by a Rauch–Tung–Striebel backward pass yields the
//! posterior state (position, tempo) at every onset time, and between onsets
//! the posterior mean is the cubic Hermite bridge between adjacent knot states
//! — piecewise cubic in position, C¹, i.e. the (windowed) cubic smoothing
//! spline. Each new onset bends the recent knots a little; the influence dies
//! off like memoryⁿ with the number of onsets in between, so values a few
//! onsets back are essentially final ("fixed-lag" smoothing). Uses: a
//! spline-true tempo visualization, a delayed-but-smooth scroll anchor, and
//! later rhythm grading against the settled curve.
//!
//! Same conventions as TempoTracker: units are caller-defined, one tuning knob
//! \p memory (γ), no dependencies. Cost is O(window) 2×2 arithmetic per onset.
class TempoSmoother
{
public:
  //! \p memory is γ ∈ (0,1) as in TempoTracker: higher = smoother. It maps to
  //! the process/measurement-noise ratio of the underlying state-space model.
  explicit TempoSmoother(double memory = 0.6);

  //! Record that at \p realTime the performance reached \p musicalPos, and
  //! re-smooth the window. Out-of-order times are ignored.
  void addObservation(double realTime, double musicalPos);

  //! Whether a smoothed curve is available yet (needs two onsets).
  bool ready() const { return _knots.size() >= 2; }

  //! The smoothed state at each retained onset time (ascending). The last knot
  //! equals the causal filter estimate; earlier knots are progressively more
  //! settled.
  struct Knot
  {
    double time;
    double position;
    double velocity;
  };
  const std::vector<Knot> &knots() const { return _knots; }

  //! Smoothed position / tempo at \p realTime. Inside the window this
  //! evaluates the spline (cubic Hermite between knots); outside it continues
  //! linearly from the nearest end knot's state. 0 before warm-up.
  double positionAt(double realTime) const;
  double velocityAt(double realTime) const;

  //! Forget all history (e.g. a position jump or a new piece).
  void reset();

private:
  void smooth();
  void evalAt(double realTime, double &position, double &velocity) const;

  const double _memory;

  struct Obs
  {
    double time;
    double pos;
  };
  std::deque<Obs> _obs;
  std::vector<Knot> _knots;
};
} // namespace dgk
