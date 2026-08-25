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

#include "PositionEstimation/PositionEstimator.h"

#include <cmath>
#include <vector>

namespace dgk
{
namespace
{
constexpr int rightHand = 0;
constexpr int leftHand = 1;
constexpr double tickStep = 480.0; // quarter notes
constexpr double interval = 500.0; // ms → 120 BPM
constexpr double t0 = 1000.0;

//! Play \p count metronomic quarters with \p hand, offsetting the onset at
//! index \p lateIndex by \p lateMs. Returns the last batch's feedback.
PositionEstimator::Feedback play(PositionEstimator &estimator, int hand,
                                 int count, int lateIndex = -1,
                                 double lateMs = 0.0)
{
  PositionEstimator::Feedback feedback;
  for (int i = 0; i < count; ++i)
  {
    const double t = t0 + i * interval + (i == lateIndex ? lateMs : 0.0);
    feedback = estimator.onOnsets(t, {{hand, i * tickStep}});
  }
  return feedback;
}

double lastOnsetTime(int count) { return t0 + (count - 1) * interval; }
} // namespace

// A metronomic performance is its own smooth curve: every onset in the window
// is judged, and judged on time.
TEST(PositionEstimatorTests, ConstantTimingIsJudgedOnTime)
{
  PositionEstimator estimator;
  const auto feedback = play(estimator, rightHand, 8);

  ASSERT_EQ(feedback.onsetTMs.count(rightHand), 1u);
  EXPECT_DOUBLE_EQ(feedback.onsetTMs.at(rightHand), lastOnsetTime(8));
  ASSERT_EQ(feedback.judgments.count(rightHand), 1u);
  const auto &window = feedback.judgments.at(rightHand);
  EXPECT_EQ(window.size(), 8u); // all of them, the first ones retroactively
  for (const auto &judgment : window)
  {
    EXPECT_NEAR(judgment.errorMs, 0.0, 1.0);
    EXPECT_NEAR(judgment.bpm, 120.0, 0.5);
  }
}

// Too few onsets to tell: a curve through two or three points fits the
// performer by construction, so no verdict is offered.
TEST(PositionEstimatorTests, NoJudgmentUntilTheWindowFills)
{
  PositionEstimator estimator;
  for (int count = 1; count <= 3; ++count)
  {
    PositionEstimator fresh;
    const auto feedback = play(fresh, rightHand, count);
    EXPECT_TRUE(feedback.judgments.empty()) << "after " << count << " onsets";
    EXPECT_EQ(feedback.onsetTMs.count(rightHand), 1u); // the mark exists
  }
}

// One note played late stands out from its neighbours by about how late it
// was, signed positive.
TEST(PositionEstimatorTests, ALateOnsetIsJudgedLate)
{
  constexpr int count = 9;
  constexpr int lateIndex = 4;
  constexpr double lateMs = 60.0;
  PositionEstimator estimator;
  const auto feedback = play(estimator, rightHand, count, lateIndex, lateMs);

  const auto &window = feedback.judgments.at(rightHand);
  ASSERT_EQ(window.size(), static_cast<std::size_t>(count));
  const double lateError = window[lateIndex].errorMs;
  EXPECT_GT(lateError, 0.5 * lateMs);
  for (std::size_t i = 0; i < window.size(); ++i)
    if (i != static_cast<std::size_t>(lateIndex))
      EXPECT_LT(std::abs(window[i].errorMs), lateError)
          << "onset " << i << " should be judged closer to the curve";
}

// Between onsets the causal estimate keeps moving at the tempo just played.
TEST(PositionEstimatorTests, EstimateAdvancesBetweenOnsets)
{
  constexpr int count = 6;
  PositionEstimator estimator;
  play(estimator, rightHand, count);

  const double last = lastOnsetTime(count);
  const double lastTick = (count - 1) * tickStep;
  ASSERT_TRUE(estimator.ready(rightHand));
  EXPECT_NEAR(*estimator.tickAt(rightHand, last), lastTick, 5.0);
  // Half an interval later, half a quarter further on.
  EXPECT_NEAR(*estimator.tickAt(rightHand, last + 0.5 * interval),
              lastTick + 0.5 * tickStep, 20.0);
  EXPECT_NEAR(*estimator.bpm(rightHand), 120.0, 2.0);
}

// A hand that stops playing coasts to a halt instead of extrapolating for
// ever.
TEST(PositionEstimatorTests, AnOverdueHandCoasts)
{
  constexpr int count = 6;
  PositionEstimator estimator;
  play(estimator, rightHand, count);
  EXPECT_FALSE(estimator.isCoasting(rightHand));

  const double last = lastOnsetTime(count);
  for (double t = last; t < last + 40 * interval; t += 16.0)
    estimator.heartbeat(t);
  EXPECT_TRUE(estimator.isCoasting(rightHand));
}

// Two hands playing the same music, one consistently behind: the asynchrony
// sample reports that lag, and says nothing when a hand is missing.
TEST(PositionEstimatorTests, AsynchronyMeasuresTheLagBetweenHands)
{
  constexpr int count = 10;
  constexpr double lagMs = 30.0;
  PositionEstimator estimator;
  for (int i = 0; i < count; ++i)
  {
    const double t = t0 + i * interval;
    estimator.onOnsets(t, {{rightHand, i * tickStep}});
    estimator.onOnsets(t + lagMs, {{leftHand, i * tickStep}});
  }

  const double now = lastOnsetTime(count) + lagMs;
  const auto sample = estimator.asynchrony(rightHand, leftHand, now);
  ASSERT_TRUE(sample.has_value());
  EXPECT_NEAR(sample->errorMs, lagMs, 12.0);
  EXPECT_FALSE(estimator.asynchrony(rightHand, 7, now).has_value());
}

// The offline re-fit judges a whole take at once, whatever its length.
TEST(PositionEstimatorTests, RefitTakeJudgesEveryOnset)
{
  std::vector<std::pair<double, double>> observations;
  for (int i = 0; i < 40; ++i)
    observations.emplace_back(t0 + i * interval, i * tickStep);

  const auto judgments = PositionEstimator::refitTake(observations, 0.6);
  EXPECT_EQ(judgments.size(), observations.size());
  for (const auto &judgment : judgments)
    EXPECT_NEAR(judgment.errorMs, 0.0, 1.0);
}
} // namespace dgk
