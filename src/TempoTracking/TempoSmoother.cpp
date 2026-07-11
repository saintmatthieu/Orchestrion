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
#include "TempoSmoother.h"

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
// Diffuse initial velocity variance: the first onset pins position only; the
// second effectively determines the velocity through the correlation this
// large prior allows. Large against the velocity information two onsets carry
// (2/dt², ≈1e-5 for ms-scale intervals) yet small enough that the covariance
// recursion doesn't cancel catastrophically (with 1e9 the smoothed curve was
// visibly polluted by float round-off).
constexpr double bigVelocityVariance = 1e4;
} // namespace

TempoSmoother::TempoSmoother(double memory, std::size_t maxKnots,
                             double maxWindowMs)
    : _memory(memory), _maxKnots(maxKnots), _maxWindowMs(maxWindowMs)
{
}

void TempoSmoother::addObservation(double realTime, double musicalPos)
{
  if (!_obs.empty() && realTime <= _obs.back().time)
    return;
  _obs.push_back({realTime, musicalPos});
  while (_obs.size() > _maxKnots ||
         _obs.back().time - _obs.front().time > _maxWindowMs)
    _obs.pop_front();
  smooth();
}

void TempoSmoother::smooth()
{
  const std::size_t n = _obs.size();
  _knots.resize(n);
  if (n == 0)
    return;
  if (n == 1)
  {
    _knots[0] = {_obs[0].time, _obs[0].pos, 0.0};
    return;
  }

  // Process-noise intensity from the one memory knob γ, via the tracking index
  // Λ = β/√(1−α) = (1−γ)²/γ that relates fading-memory gains to the
  // steady-state Kalman filter of this model. Only the ratio to the (unit)
  // measurement variance matters; the mean onset interval sets the time scale.
  const double meanDt =
      std::max((_obs.back().time - _obs.front().time) / (n - 1), 1.0);
  const double trackingIndex = (1.0 - _memory) * (1.0 - _memory) / _memory;
  const double q = trackingIndex * trackingIndex / (meanDt * meanDt * meanDt);

  // Forward Kalman pass over the window. State (s, v); constant-velocity
  // transition with white-noise acceleration of intensity q; position
  // measurements with unit variance. Covariances are symmetric 2×2, kept as
  // (P00, P01, P11).
  struct Step
  {
    double s, v, P00, P01, P11;     // filtered (posterior)
    double sp, vp, Q00, Q01, Q11;   // predicted (prior), for the backward pass
  };
  std::vector<Step> fwd(n);
  fwd[0] = {_obs[0].pos, 0.0, 1.0, 0.0, bigVelocityVariance, 0, 0, 0, 0, 0};
  for (std::size_t k = 1; k < n; ++k)
  {
    const Step &prev = fwd[k - 1];
    Step &cur = fwd[k];
    const double dt = _obs[k].time - _obs[k - 1].time;

    // Predict.
    cur.sp = prev.s + prev.v * dt;
    cur.vp = prev.v;
    cur.Q00 = prev.P00 + dt * (2.0 * prev.P01 + dt * prev.P11) +
              q * dt * dt * dt / 3.0;
    cur.Q01 = prev.P01 + dt * prev.P11 + q * dt * dt / 2.0;
    cur.Q11 = prev.P11 + q * dt;

    // Correct with the onset position (measurement variance 1).
    const double S = cur.Q00 + 1.0;
    const double K0 = cur.Q00 / S;
    const double K1 = cur.Q01 / S;
    const double y = _obs[k].pos - cur.sp;
    cur.s = cur.sp + K0 * y;
    cur.v = cur.vp + K1 * y;
    cur.P00 = (1.0 - K0) * cur.Q00;
    cur.P01 = (1.0 - K0) * cur.Q01;
    cur.P11 = cur.Q11 - K1 * cur.Q01;
  }

  // Backward Rauch–Tung–Striebel pass: fold each later knot's correction into
  // the earlier ones through the smoother gain C = P Fᵀ (prior at k+1)⁻¹.
  _knots[n - 1] = {_obs[n - 1].time, fwd[n - 1].s, fwd[n - 1].v};
  for (std::size_t k = n - 1; k-- > 0;)
  {
    const Step &f = fwd[k];
    const Step &next = fwd[k + 1];
    const double dt = _obs[k + 1].time - _obs[k].time;
    const double A00 = f.P00 + dt * f.P01;
    const double A01 = f.P01;
    const double A10 = f.P01 + dt * f.P11;
    const double A11 = f.P11;
    const double det = next.Q00 * next.Q11 - next.Q01 * next.Q01;
    const double C00 = (A00 * next.Q11 - A01 * next.Q01) / det;
    const double C01 = (A01 * next.Q00 - A00 * next.Q01) / det;
    const double C10 = (A10 * next.Q11 - A11 * next.Q01) / det;
    const double C11 = (A11 * next.Q00 - A10 * next.Q01) / det;
    const double ds = _knots[k + 1].position - next.sp;
    const double dv = _knots[k + 1].velocity - next.vp;
    _knots[k] = {_obs[k].time, f.s + C00 * ds + C01 * dv,
                 f.v + C10 * ds + C11 * dv};
  }
}

void TempoSmoother::evalAt(double realTime, double &position,
                           double &velocity) const
{
  if (_knots.empty())
  {
    position = 0.0;
    velocity = 0.0;
    return;
  }
  const Knot &first = _knots.front();
  const Knot &last = _knots.back();
  if (_knots.size() == 1 || realTime <= first.time || realTime >= last.time)
  {
    // Outside the window: continue linearly from the nearest end state.
    const Knot &end = realTime <= first.time ? first : last;
    position = end.position + end.velocity * (realTime - end.time);
    velocity = end.velocity;
    return;
  }

  const auto it =
      std::upper_bound(_knots.begin(), _knots.end(), realTime,
                       [](double t, const Knot &k) { return t < k.time; });
  const Knot &b = *it;
  const Knot &a = *std::prev(it);

  // Between knots the posterior mean is the cubic Hermite bridge matching both
  // knot states — this is what makes the curve the C¹ piecewise-cubic spline.
  const double h = b.time - a.time;
  const double u = (realTime - a.time) / h;
  const double h00 = (1.0 + 2.0 * u) * (1.0 - u) * (1.0 - u);
  const double h10 = u * (1.0 - u) * (1.0 - u);
  const double h01 = u * u * (3.0 - 2.0 * u);
  const double h11 = u * u * (u - 1.0);
  position = h00 * a.position + h10 * h * a.velocity + h01 * b.position +
             h11 * h * b.velocity;
  const double d00 = 6.0 * u * u - 6.0 * u;
  const double d10 = 3.0 * u * u - 4.0 * u + 1.0;
  const double d01 = -d00;
  const double d11 = 3.0 * u * u - 2.0 * u;
  velocity = (d00 * a.position + d10 * h * a.velocity + d01 * b.position +
              d11 * h * b.velocity) /
             h;
}

double TempoSmoother::positionAt(double realTime) const
{
  double position = 0.0, velocity = 0.0;
  evalAt(realTime, position, velocity);
  return position;
}

double TempoSmoother::velocityAt(double realTime) const
{
  double position = 0.0, velocity = 0.0;
  evalAt(realTime, position, velocity);
  return velocity;
}

std::vector<TempoSmoother::Residual> TempoSmoother::residuals() const
{
  // _knots[i] is the smoothed state at _obs[i]'s time (smooth() keeps them in
  // lockstep).
  std::vector<Residual> result;
  result.reserve(_knots.size());
  for (std::size_t i = 0; i < _knots.size(); ++i)
    result.push_back({_knots[i].time, _obs[i].pos - _knots[i].position,
                      _knots[i].velocity});
  return result;
}

void TempoSmoother::reset()
{
  _obs.clear();
  _knots.clear();
}
} // namespace dgk
