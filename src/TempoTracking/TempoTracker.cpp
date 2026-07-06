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

#include <cmath>

namespace dgk
{
namespace
{
// Coast trigger: an onset is "overdue" (performer stopped, not merely slowing)
// only once it is this many inter-onset intervals late. Above 1 so ordinary
// ritardandi don't trip the coast.
constexpr double coastTrigger = 15;

// Coast deceleration time constant as a fraction of the inter-onset interval:
// once overdue, the tempo decays with this time constant, easing to a stop.
constexpr double coastFactor = 0.9;

// Decay time constant (ms) for the display offset that absorbs jumps the α–β
// correction would otherwise make (e.g. centring a long-overdue note): the view
// slides to the corrected position over a few of these, gradually.
constexpr double offsetDecayMs = 1000.0;
} // namespace

TempoTracker::TempoTracker(double memory)
    // Fading-memory α–β gains from one knob γ = memory: critically damped, so
    // no overshoot and no separate variances to tune.
    : _alpha(1.0 - memory * memory), _beta((1.0 - memory) * (1.0 - memory))
{
}

void TempoTracker::advanceTo(double realTime)
{
  const double dt = realTime - _lastTime;
  if (dt <= 0.0)
    return;
  _p += _v * dt;
  _lastTime = realTime;
}

void TempoTracker::addObservation(double realTime, double musicalPos)
{
  if (_nObs == 0)
  {
    // First onset: position known, tempo not yet.
    _p = musicalPos;
    _v = 0.0;
    _lastTime = realTime;
    _lastObsTime = realTime;
    _nObs = 1;
    return;
  }

  // Where the output currently sits (offset included), to keep it continuous.
  const bool wasReady = ready();
  const double prev = positionAt(realTime);

  advanceTo(realTime);
  const double gap = realTime - _lastObsTime;
  const double y = musicalPos - _p; // innovation (the timing error)

  if (_nObs == 1)
  {
    // Second onset: seed the tempo directly from the two positions (the α–β
    // ramp-up would otherwise start far too slow).
    if (gap > 0.0)
      _v = (musicalPos - _p) / gap;
    _p = musicalPos;
    _nObs = 2;
  }
  else
  {
    // α–β correction. β is per-interval, so divide by the gap for the tempo
    // adjustment per unit time.
    _p += _alpha * y;
    if (gap > 0.0)
      _v += _beta * y / gap;
    ++_nObs;
  }

  // Track the playing cadence, ignoring pause-length gaps so a stop doesn't
  // inflate the "overdue" threshold.
  if (gap > 0.0 && (_intervalMs <= 0.0 || gap < 2.0 * _intervalMs))
    _intervalMs = _intervalMs > 0.0 ? 0.5 * _intervalMs + 0.5 * gap : gap;

  _lastObsTime = realTime;
  _lastTime = realTime;
  _coasting = false;

  // Absorb the jump this correction made into a decaying offset, so the view
  // slides to the new position gradually instead of snapping.
  _displayOffset0 = wasReady ? prev - _p : 0.0;
  _offsetT0 = realTime;
}

void TempoTracker::heartbeat(double realTime)
{
  if (_nObs < 2)
    return;
  const double dt = realTime - _lastTime;
  if (dt <= 0.0)
    return;

  _p += _v * dt;

  const bool overdue = _lastObsTime >= 0.0 && _intervalMs > 0.0 &&
                       (realTime - _lastObsTime) > coastTrigger * _intervalMs;
  if (overdue)
    _v *= std::exp(-dt / (coastFactor * _intervalMs));

  _lastTime = realTime;
  _coasting = overdue;
}

double TempoTracker::baseAt(double realTime) const
{
  return _p + _v * (realTime - _lastTime);
}

double TempoTracker::offsetAt(double realTime) const
{
  if (_displayOffset0 == 0.0 || realTime <= _offsetT0)
    return _displayOffset0;
  return _displayOffset0 * std::exp(-(realTime - _offsetT0) / offsetDecayMs);
}

double TempoTracker::positionAt(double realTime) const
{
  if (_nObs < 2)
    return _nObs == 0 ? 0.0 : _p;
  return baseAt(realTime) + offsetAt(realTime);
}

void TempoTracker::reset()
{
  _p = 0.0;
  _v = 0.0;
  _lastTime = 0.0;
  _lastObsTime = -1.0;
  _intervalMs = 0.0;
  _nObs = 0;
  _coasting = false;
  _displayOffset0 = 0.0;
  _offsetT0 = 0.0;
}
} // namespace dgk
