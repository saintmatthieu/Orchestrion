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

#include "TempoTracking/TempoTracker.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace dgk
{
namespace
{
constexpr double ticksPerQuarter = 480.0;
constexpr double bpmFactor = 60000.0 / ticksPerQuarter; // ticks/ms -> BPM

// Drive the tracker exactly as TempoFollower::tick() does: onsets are fed at
// their (caller-supplied) times while heartbeat() is sampled at ~60 fps, and the
// per-frame tempo readout (speed()*bpmFactor) is collected once the tracker is
// ready.
std::vector<double> simulateBpm(const std::vector<double> &onsetTimesMs,
                                double tickStep)
{
  TempoTracker tracker; // default memory, as the follower uses
  std::vector<double> bpm;
  std::size_t next = 0;
  for (double now = 0.0; now <= onsetTimesMs.back() + 1e-6; now += 16.0)
  {
    while (next < onsetTimesMs.size() && onsetTimesMs[next] <= now + 1e-9)
    {
      tracker.addObservation(onsetTimesMs[next],
                             static_cast<double>(next) * tickStep);
      ++next;
    }
    tracker.heartbeat(now);
    if (tracker.ready())
      bpm.push_back(tracker.speed() * bpmFactor);
  }
  return bpm;
}

// Tempo samples after the warm-up frames.
std::vector<double> steadyTail(const std::vector<double> &bpm)
{
  const auto skip = std::min<std::size_t>(40, bpm.size());
  return {bpm.begin() + skip, bpm.end()};
}
} // namespace

// Gestures exactly on time ⇒ the output tempo curve is constant.
TEST(TempoTrackerTests, ConstantTimingYieldsConstantTempo)
{
  constexpr double tickStep = 240.0; // eighth notes
  constexpr double interval = 300.0; // ms  → 100 BPM
  const double expected = tickStep / interval * bpmFactor;

  std::vector<double> onsets;
  for (int i = 0; i < 40; ++i)
    onsets.push_back(i * interval);

  const std::vector<double> tail = steadyTail(simulateBpm(onsets, tickStep));
  ASSERT_FALSE(tail.empty());
  const auto [lo, hi] = std::minmax_element(tail.begin(), tail.end());

  EXPECT_LT(*hi - *lo, 1e-6) << "tempo curve should be flat for exact timing";
  EXPECT_NEAR(*hi, expected, 1e-6);
}

// The same notes with timing jitter ⇒ the curve is no longer flat. This is what
// produces the "dents" in the app: onset-timing jitter, not the model.
TEST(TempoTrackerTests, TimingJitterShowsInTempo)
{
  constexpr double tickStep = 240.0;
  constexpr double interval = 300.0;

  std::vector<double> onsets;
  double t = 0.0;
  for (int i = 0; i < 40; ++i)
  {
    onsets.push_back(t);
    t += interval + 0.05 * interval * std::sin(i * 1.3); // deterministic ±5%
  }

  const std::vector<double> tail = steadyTail(simulateBpm(onsets, tickStep));
  ASSERT_FALSE(tail.empty());
  const auto [lo, hi] = std::minmax_element(tail.begin(), tail.end());

  EXPECT_GT(*hi - *lo, 0.5) << "jittered input should perturb the tempo curve";
}
} // namespace dgk
