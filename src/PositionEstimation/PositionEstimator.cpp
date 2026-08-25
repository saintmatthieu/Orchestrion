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
#include "PositionEstimator.h"

#include <cmath>
#include <limits>

namespace dgk
{
namespace
{
// Score ticks per quarter note (MuseScore's division), to express a tempo in
// BPM.
constexpr double ticksPerQuarter = 480.0;

// No judgments until the smoothing window holds this many onsets: a curve
// through two or three points fits the performer nearly by construction, so
// its residuals are flattery, not information. Once past this, *all* window
// onsets are judged (the first ones included, retroactively).
constexpr std::size_t judgeMinKnots = 4;

// The asynchrony sample compares the hands' curves this many cadence steps in
// the past, where both have settled.
constexpr double syncLagIntervals = 2.0;

// Sampling step (ms) of a plotted smoothed tempo curve.
constexpr double curveStepMs = 40.0;

double toBpm(double ticksPerMs)
{
  return ticksPerMs * 60000.0 / ticksPerQuarter;
}

//! (Re-)measure every onset in the smoother's window against the curve as it
//! now stands. A smooth tempo bend is part of the curve, not an error, and
//! each onset's verdict keeps refining as later ones lend it hindsight.
std::vector<PositionEstimator::Judgment>
judgeWindow(const PositionSmoother &smoother)
{
  std::vector<PositionEstimator::Judgment> window;
  if (smoother.knots().size() < judgeMinKnots)
    return window;
  const auto &knots = smoother.knots();
  const auto residuals = smoother.residuals();
  // The window's constant-tempo reference: the straight line through its end
  // knots — the timeline a time-proportional layout would represent.
  const double t0 = knots.front().time;
  const double p0 = knots.front().position;
  const double refSpan = knots.back().time - t0;
  const double refTempo =
      refSpan > 0.0 ? (knots.back().position - p0) / refSpan : 0.0;
  window.reserve(residuals.size());
  for (std::size_t i = 0; i < residuals.size(); ++i)
  {
    const auto &r = residuals[i];
    if (r.velocity <= 1e-9)
      continue;
    // Residual in ticks, negated into an arrival-time error: a note whose
    // tick the curve hasn't reached yet arrived early (− ms).
    const double errorMs = -r.error / r.velocity;
    // The tempo warp: where the fitted arrival time falls on the reference
    // timeline, minus the notated tick — the smooth part of the note's
    // displacement; the residual is the jittery rest.
    const double fittedArrival = r.time - errorMs;
    const double notatedTick = knots[i].position + r.error;
    const double warpTicks = p0 + refTempo * (fittedArrival - t0) - notatedTick;
    window.push_back({r.time, errorMs, warpTicks, refTempo * errorMs,
                      refTempo > 1e-9 ? warpTicks / refTempo : 0.0,
                      toBpm(r.velocity)});
  }
  return window;
}
} // namespace

PositionEstimator::PositionEstimator(double memory) : _memory{memory} {}

void PositionEstimator::setMemory(double memory) { _memory = memory; }

PositionEstimator::Feedback
PositionEstimator::onOnsets(double nowMs, const std::map<int, double> &onsets)
{
  Feedback feedback;
  for (const auto &[staff, utick] : onsets)
  {
    Hand &hand = _hands.try_emplace(staff, Hand{_memory}).first->second;
    if (hand.hasOnset && utick == hand.lastUtick)
      continue; // the same onset again (another voice of the staff)
    hand.lastUtick = utick;
    hand.hasOnset = true;

    // A coast means the performer stopped; the wound-down curve says nothing
    // about the tempo they resume at, so the estimate starts over. (Uticks
    // themselves need no such care: they run monotonically through repeats
    // and jumps.)
    if (hand.tracker.isCoasting())
    {
      hand.tracker.reset();
      hand.smoother.reset();
    }

    hand.tracker.addObservation(nowMs, utick);
    hand.smoother.addObservation(nowMs, utick);

    feedback.onsetTMs[staff] = nowMs;
    auto window = judgeWindow(hand.smoother);
    if (!window.empty())
      feedback.judgments[staff] = std::move(window);
  }
  return feedback;
}

void PositionEstimator::heartbeat(double nowMs)
{
  for (auto &[staff, hand] : _hands)
    hand.tracker.heartbeat(nowMs);
}

const PositionEstimator::Hand *PositionEstimator::find(int staff) const
{
  const auto it = _hands.find(staff);
  return it == _hands.end() ? nullptr : &it->second;
}

bool PositionEstimator::ready(int staff) const
{
  const Hand *hand = find(staff);
  return hand && hand->tracker.ready();
}

std::optional<double> PositionEstimator::tickAt(int staff, double nowMs) const
{
  const Hand *hand = find(staff);
  if (!hand || !hand->tracker.ready())
    return std::nullopt;
  return hand->tracker.positionAt(nowMs);
}

std::optional<double> PositionEstimator::bpm(int staff) const
{
  const Hand *hand = find(staff);
  if (!hand || !hand->tracker.ready())
    return std::nullopt;
  return toBpm(hand->tracker.speed());
}

bool PositionEstimator::isCoasting(int staff) const
{
  const Hand *hand = find(staff);
  return hand && hand->tracker.isCoasting();
}

std::vector<PositionEstimator::CurvePoint>
PositionEstimator::smoothedBpmCurve(int staff) const
{
  std::vector<CurvePoint> curve;
  const Hand *hand = find(staff);
  if (!hand || !hand->smoother.ready())
    return curve;
  const auto &knots = hand->smoother.knots();
  const double tBegin = knots.front().time;
  const double tEnd = knots.back().time;
  curve.reserve(static_cast<std::size_t>((tEnd - tBegin) / curveStepMs) + 2);
  for (double t = tBegin; t < tEnd; t += curveStepMs)
    curve.push_back({t, toBpm(hand->smoother.velocityAt(t))});
  curve.push_back({tEnd, toBpm(hand->smoother.velocityAt(tEnd))});
  return curve;
}

std::optional<PositionEstimator::Judgment>
PositionEstimator::asynchrony(int leadingStaff, int laggingStaff,
                              double nowMs) const
{
  const Hand *leading = find(leadingStaff);
  const Hand *lagging = find(laggingStaff);
  if (!leading || !lagging)
    return std::nullopt;
  for (const Hand *hand : {leading, lagging})
    if (!hand->smoother.ready() || hand->tracker.isCoasting() ||
        hand->tracker.intervalMs() <= 0.0)
      return std::nullopt;

  // The musical positions the two curves assign to the same instant should
  // agree; the gap, divided by the tempo, is the asynchrony in time. Sampled
  // a couple of cadence steps back, where both curves have settled.
  const double intervalSum =
      leading->tracker.intervalMs() + lagging->tracker.intervalMs();
  const double tEval = nowMs - syncLagIntervals * 0.5 * intervalSum;
  const double meanVelocity = 0.5 * (leading->smoother.velocityAt(tEval) +
                                     lagging->smoother.velocityAt(tEval));
  if (meanVelocity <= 1e-9)
    return std::nullopt;
  Judgment sample;
  sample.tMs = nowMs;
  sample.errorMs = (leading->smoother.positionAt(tEval) -
                    lagging->smoother.positionAt(tEval)) /
                   meanVelocity;
  return sample;
}

void PositionEstimator::reset() { _hands.clear(); }

std::vector<PositionEstimator::Judgment> PositionEstimator::refitTake(
    const std::vector<std::pair<double, double>> &observations, double memory)
{
  PositionSmoother smoother(memory, std::numeric_limits<std::size_t>::max(),
                            std::numeric_limits<double>::max());
  for (const auto &[tMs, utick] : observations)
    smoother.addObservation(tMs, utick);
  return judgeWindow(smoother);
}
} // namespace dgk
