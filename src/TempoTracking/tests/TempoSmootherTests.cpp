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
#include <gtest/gtest.h>

#include "TempoTracking/TempoSmoother.h"
#include "TempoTracking/TempoTracker.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace dgk
{
namespace
{
constexpr double tickStep = 240.0; // eighth notes
constexpr double interval = 300.0; // ms → 100 BPM
constexpr double trueTempo = tickStep / interval;

// Onset times with a deterministic relative timing jitter (0 = exact).
std::vector<double> onsetTimes(int count, double jitterFrac)
{
  std::vector<double> times;
  double t = 1000.0;
  for (int i = 0; i < count; ++i)
  {
    times.push_back(t);
    t += interval + jitterFrac * interval * std::sin(i * 1.3);
  }
  return times;
}
} // namespace

// Gestures exactly on time ⇒ the smoothed curve is the exact line: constant
// tempo at every knot *and* everywhere between them (the Hermite bridges).
TEST(TempoSmootherTests, ConstantTimingYieldsConstantSmoothedTempo)
{
  TempoSmoother smoother;
  const std::vector<double> times = onsetTimes(20, 0.0);
  for (std::size_t k = 0; k < times.size(); ++k)
    smoother.addObservation(times[k], static_cast<double>(k) * tickStep);
  ASSERT_TRUE(smoother.ready());

  double lo = std::numeric_limits<double>::max();
  double hi = std::numeric_limits<double>::lowest();
  for (double t = smoother.knots().front().time;
       t <= smoother.knots().back().time; t += 10.0)
  {
    const double v = smoother.velocityAt(t);
    lo = std::min(lo, v);
    hi = std::max(hi, v);
  }
  EXPECT_LT(hi - lo, 1e-6) << "smoothed tempo should be flat for exact timing";
  EXPECT_NEAR(hi, trueTempo, 1e-6);
}

// With hindsight the smoother attenuates timing jitter more than the causal
// filter: the settled (interior) knots wobble less around the true tempo than
// the filter's running estimate over the same performance.
TEST(TempoSmootherTests, SmoothsTimingJitterBetterThanTheFilter)
{
  const std::vector<double> times = onsetTimes(30, 0.05);

  TempoSmoother smoother;
  TempoTracker tracker;
  double filterWobble = 0.0;
  for (std::size_t k = 0; k < times.size(); ++k)
  {
    const double pos = static_cast<double>(k) * tickStep;
    smoother.addObservation(times[k], pos);
    tracker.addObservation(times[k], pos);
    if (k >= 4) // past the tracker's velocity seeding
      filterWobble =
          std::max(filterWobble, std::abs(tracker.speed() - trueTempo));
  }
  ASSERT_TRUE(smoother.ready());

  const auto &knots = smoother.knots();
  double smoothedWobble = 0.0;
  for (std::size_t k = 4; k + 2 < knots.size(); ++k) // settled knots only
    smoothedWobble =
        std::max(smoothedWobble, std::abs(knots[k].velocity - trueTempo));

  EXPECT_GT(filterWobble, 0.0);
  EXPECT_LT(smoothedWobble, filterWobble)
      << "hindsight should beat the causal estimate";
  EXPECT_LT(smoothedWobble, 0.03 * trueTempo);
}

// The curve is C¹: position and tempo are continuous across the knots (the
// Hermite bridges meet the knot states exactly).
TEST(TempoSmootherTests, SmoothedCurveIsContinuousAcrossKnots)
{
  TempoSmoother smoother;
  const std::vector<double> times = onsetTimes(20, 0.05);
  for (std::size_t k = 0; k < times.size(); ++k)
    smoother.addObservation(times[k], static_cast<double>(k) * tickStep);
  ASSERT_TRUE(smoother.ready());

  constexpr double eps = 1e-3; // ms
  const auto &knots = smoother.knots();
  for (std::size_t k = 1; k + 1 < knots.size(); ++k)
  {
    const double t = knots[k].time;
    // The curve legitimately advances by ≈ v·2ε across the straddle; only the
    // remainder would be a position jump.
    EXPECT_NEAR(smoother.positionAt(t + eps) - smoother.positionAt(t - eps),
                2.0 * eps * smoother.velocityAt(t), 1e-6);
    EXPECT_NEAR(smoother.velocityAt(t - eps), smoother.velocityAt(t + eps),
                1e-4);
  }
}
} // namespace dgk
