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
#include "TempoTracker.h"

#include <cassert>
#include <cmath>

namespace dgk
{
namespace
{
constexpr double pi = 3.14159265358979323846;

// Coast (deceleration) time constant as a fraction of the inter-onset interval:
// the view eases to a stop over a few of these once the next note is overdue.
constexpr double coastFactor = 0.9;

// Decay time constant (ms) for the display offset that absorbs discontinuities
// in the fitted position (e.g. when a late note snaps the fit back): the view
// re-converges to the true position over a few of these, gradually.
constexpr double offsetDecayMs = 250.0;

//! Recency weight for observation \p index of \p numObs (oldest = 0), over a
//! window of \p ageLimit slots. Ported verbatim from Christophone's
//! tempo-driven controller: a raised cosine whose apex is shifted toward the
//! newest observations (so the freshest tempo dominates while older ones taper
//! off smoothly rather than being cut off). Requires numObs <= ageLimit, which
//! the fixed-size history guarantees.
double observationWeight(int numObs, int index, int ageLimit)
{
  constexpr double cosApexPosition = 0.9;
  const double omega = 2.0 * pi / (ageLimit + 2);
  constexpr double A = 1.0 - cosApexPosition;
  const int J = ageLimit;

  const int j = J - numObs + index;
  assert(0 <= j && j < J);
  const double warpedJ =
      (j <= J / 2) ? 2.0 * A * j : (2.0 * (1.0 - A) * j + (2.0 * A - 1.0) * J);
  return (1.0 - std::cos((warpedJ + 1.0) * omega)) / 2.0;
}
} // namespace

TempoTracker::TempoTracker(int maxObservations, int minObservations)
    : _maxObservations(maxObservations), _minObservations(minObservations)
{
  assert(_maxObservations >= 2);
  assert(_minObservations >= 2 && _minObservations <= _maxObservations);
}

void TempoTracker::addObservation(double realTime, double musicalPos)
{
  // Where the output sits right now (coast/overshoot included), so we can keep
  // it continuous across the refit below.
  const bool wasReady = _ready;
  const double prev = positionAt(realTime);

  _coasting = false; // a real onset resumes live tracking
  _observations.emplace_back(realTime, musicalPos);
  while (static_cast<int>(_observations.size()) > _maxObservations)
    _observations.pop_front();
  refit();

  // Absorb any jump the refit introduced into a decaying offset, so the view
  // slides to the new (centered) position gradually instead of snapping. Only
  // when we were already tracking — at warm-up we want a clean start.
  if (wasReady && _ready)
  {
    _displayOffset0 = prev - baseAt(realTime);
    _offsetT0 = realTime;
  }
  else
    _displayOffset0 = 0.0;
}

void TempoTracker::heartbeat(double realTime)
{
  const int n = static_cast<int>(_observations.size());
  if (n < 2)
    return;

  const double lastTime = _observations.back().first;
  // Expected cadence = the mean inter-onset interval over the window.
  const double interval = (lastTime - _observations.front().first) / (n - 1);
  if (interval <= 0.0)
    return;

  if (realTime > lastTime + interval)
  {
    // The next onset is overdue: begin (or continue) coasting. Capture the
    // current position and tempo once; positionAt() then decays the velocity so
    // the view eases to a monotonic stop instead of extrapolating off the end.
    if (!_coasting)
    {
      _coasting = true;
      _coastT0 = realTime;
      _coastP0 = _slope * (realTime - _refTime) + _intercept;
      _coastV0 = _slope;
      _coastTau = coastFactor * interval;
    }
  }
  else
    _coasting = false;
}

void TempoTracker::refit()
{
  const int n = static_cast<int>(_observations.size());
  if (n < _minObservations)
  {
    _ready = false;
    return;
  }

  // Shift realTime so the regression runs near the origin (keeps the normal
  // equations well-conditioned even when realTime is a large clock value).
  _refTime = _observations.front().first;

  // Weighted least-squares fit of y = slope·x + intercept via the 2×2 normal
  // equations (replacing the Eigen solve of the original).
  double sumW = 0.0, sumX = 0.0, sumY = 0.0, sumXX = 0.0, sumXY = 0.0;
  for (int i = 0; i < n; ++i)
  {
    const double w = observationWeight(n, i, _maxObservations);
    const double x = _observations[i].first - _refTime;
    const double y = _observations[i].second;
    sumW += w;
    sumX += w * x;
    sumY += w * y;
    sumXX += w * x * x;
    sumXY += w * x * y;
  }

  const double denom = sumW * sumXX - sumX * sumX;
  if (std::abs(denom) < 1e-12)
  {
    // Degenerate (e.g. all observations at the same realTime); keep the last
    // good fit rather than dividing by ~0.
    return;
  }

  _slope = (sumW * sumXY - sumX * sumY) / denom;
  _intercept = (sumXX * sumY - sumX * sumXY) / denom;
  _ready = true;
}

double TempoTracker::baseAt(double realTime) const
{
  if (_coasting && realTime > _coastT0)
    // Decaying-velocity coast: eases monotonically from _coastP0 to the rest
    // position _coastP0 + _coastV0·_coastTau.
    return _coastP0 + _coastV0 * _coastTau *
                          (1.0 - std::exp(-(realTime - _coastT0) / _coastTau));
  return _slope * (realTime - _refTime) + _intercept;
}

double TempoTracker::offsetAt(double realTime) const
{
  if (_displayOffset0 == 0.0 || realTime <= _offsetT0)
    return _displayOffset0;
  return _displayOffset0 * std::exp(-(realTime - _offsetT0) / offsetDecayMs);
}

double TempoTracker::positionAt(double realTime) const
{
  if (!_ready)
    return _observations.empty() ? 0.0 : _observations.back().second;
  return baseAt(realTime) + offsetAt(realTime);
}

void TempoTracker::reset()
{
  _observations.clear();
  _coasting = false;
  _displayOffset0 = 0.0;
  _offsetT0 = 0.0;
  _slope = 0.0;
  _intercept = 0.0;
  _refTime = 0.0;
  _ready = false;
}
} // namespace dgk
