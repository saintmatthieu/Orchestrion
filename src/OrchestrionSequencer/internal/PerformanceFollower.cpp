/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "PerformanceFollower.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>

namespace dgk
{
namespace
{
constexpr double minTempoRatio = 0.25;
constexpr double maxTempoRatio = 4.0;
/**
 * A tempo measurement this far off the estimate is taken for a
 * misalignment rather than a tempo change.
 */
constexpr double maxTempoStep = 2.0;
/** States this much less likely than the likeliest aren't propagated. */
constexpr double pruning = 1e-12;
/** MuseScore's. */
constexpr double ticksPerQuarter = 480.0;
} // namespace

PerformanceFollower::PerformanceFollower(
    std::vector<ScoreEvent> events, double nominalTicksPerMs,
    PerformanceFollowerParameters parameters)
    : m_events{std::move(events)}, m_p{parameters},
      m_ticksPerMs{nominalTicksPerMs}, m_nominalTicksPerMs{nominalTicksPerMs}
{
  assert(!m_events.empty());
  assert(nominalTicksPerMs > 0);
  assert(std::adjacent_find(m_events.begin(), m_events.end(),
                            [](const ScoreEvent &a, const ScoreEvent &b)
                            { return a.tick >= b.tick; }) == m_events.end());

  m_jumpWeights.assign(m_events.size(), 0.0);
  m_posterior.assign(m_events.size(), 0.0);
  m_prediction.assign(m_events.size(), 0.0);
  m_stayed.assign(m_events.size(), 0.0);
  m_onsetMs.assign(m_events.size(), 0.0);
  Reset(0);
}

void PerformanceFollower::Reset(size_t event)
{
  m_fresh = true;
  m_start = std::min(event, m_events.size() - 1);
  m_followed = m_start;
  m_lastBest = m_start;
  m_hasAnchor = false;
}

PerformanceFollower::Estimate PerformanceFollower::OnNoteOn(int pitch,
                                                            double ms)
{
  if (m_fresh)
    PredictFrom(m_start, ms);
  else
    Predict(ms);

  double sum = 0.0;
  for (auto j = m_begin; j < m_end; ++j)
  {
    m_posterior[j] = m_prediction[j] * Emission(pitch, m_events[j]);
    sum += m_posterior[j];
  }
  // The followed event has some jump mass and every pitch some emission:
  // never 0.
  assert(sum > 0);
  for (auto j = m_begin; j < m_end; ++j)
    m_posterior[j] /= sum;

  const auto best = static_cast<size_t>(
      std::max_element(m_posterior.begin() + m_begin,
                       m_posterior.begin() + m_end) -
      m_posterior.begin());
  const Estimate estimate{best, m_posterior[best]};

  if (estimate.probability >= m_p.followThreshold)
  {
    m_followed = best;
    if (best != m_lastBest || !m_hasAnchor)
      UpdateTempo(best, ms);
    m_lastBest = best;
  }

  m_fresh = false;
  m_lastMs = ms;
  return estimate;
}

double PerformanceFollower::Emission(int pitch, const ScoreEvent &event) const
{
  auto e = m_p.noise;
  for (const auto p : event.pitches)
  {
    const auto distance = std::abs(pitch - p);
    if (distance == 0)
      return m_p.match;
    if (distance <= 2)
      e = std::max(e, m_p.nearMiss);
    else if (distance == 12)
      e = std::max(e, m_p.octave);
  }
  return e;
}

double PerformanceFollower::Timing(int ticks, double dtMs) const
{
  const auto floor = m_p.timingFloorMs * m_ticksPerMs;
  const auto expected = std::max(dtMs, 0.0) * m_ticksPerMs;
  const auto r =
      std::log((ticks + floor) / (expected + floor)) / m_p.timingSigma;
  return std::exp(-0.5 * r * r);
}

double PerformanceFollower::Stay(size_t event, double sinceOnsetMs) const
{
  const auto chord =
      m_p.chordMember * std::exp(-sinceOnsetMs / m_p.chordSpreadMs);
  if (!m_events[event].trill)
    return chord;
  if (event + 1 == m_events.size())
    return std::max(chord, m_p.trillNote);
  const auto length = m_events[event + 1].tick - m_events[event].tick;
  const auto elapsed = sinceOnsetMs * m_ticksPerMs / length;
  return std::max(chord, m_p.trillNote /
                             (1.0 + std::pow(elapsed, m_p.trillEndSharpness)));
}

void PerformanceFollower::Predict(double ms)
{
  const auto n = m_events.size();
  const auto paused = ms - m_lastMs > m_p.pauseMs;
  const auto jump = paused ? m_p.jumpAfterPause : m_p.jump;
  const auto maxPosterior =
      *std::max_element(m_posterior.begin() + m_begin,
                        m_posterior.begin() + m_end);

  std::fill(m_prediction.begin() + m_begin, m_prediction.begin() + m_end,
            0.0);
  std::fill(m_stayed.begin() + m_begin, m_stayed.begin() + m_end, 0.0);
  auto first = m_end;
  auto last = m_begin;
  std::vector<double> w(m_p.maxAdvance + 1);
  for (auto i = m_begin; i < m_end; ++i)
  {
    const auto a = m_posterior[i];
    if (a < pruning * maxPosterior)
      continue;
    first = std::min(first, i);
    last = i;
    // Timed from the event's onset rather than from the previous note, so
    // that an event of many notes (a trill) is left when its time is up.
    const auto sinceOnset = ms - m_onsetMs[i];
    w[0] = m_p.insertion + (paused ? 0.0 : Stay(i, sinceOnset));
    auto sum = w[0];
    auto advance = m_p.advance;
    size_t k = 1;
    for (; k < w.size() && i + k < n; ++k)
    {
      const auto ticks = m_events[i + k].tick - m_events[i].tick;
      w[k] = advance * (paused ? 1.0 : Timing(ticks, sinceOnset));
      sum += w[k];
      advance *= m_p.skip;
    }
    const auto local = a * (1.0 - jump) / sum;
    m_stayed[i] = local * w[0];
    for (size_t l = 0; l < k; ++l)
      m_prediction[i + l] += local * w[l];
  }
  std::fill(m_posterior.begin() + m_begin, m_posterior.begin() + m_end, 0.0);
  const auto [jumpBegin, jumpEnd] = JumpSpan();
  m_begin = std::min(first, jumpBegin);
  m_end = std::max(std::min(n, last + m_p.maxAdvance + 1), jumpEnd);
  AddJumps(jump);

  // An event mostly arrived at, rather than stayed on, begins now.
  for (auto j = m_begin; j < m_end; ++j)
    if (m_stayed[j] < 0.5 * m_prediction[j])
      m_onsetMs[j] = ms;
}

void PerformanceFollower::PredictFrom(size_t start, double ms)
{
  const auto n = m_events.size();
  std::fill(m_prediction.begin() + m_begin, m_prediction.begin() + m_end,
            0.0);
  std::fill(m_stayed.begin() + m_begin, m_stayed.begin() + m_end, 0.0);
  std::fill(m_posterior.begin() + m_begin, m_posterior.begin() + m_end, 0.0);
  const auto end = std::min(n, start + m_p.maxAdvance + 1);
  auto w = 1.0;
  auto sum = 0.0;
  for (auto j = start; j < end; ++j)
  {
    m_prediction[j] = w;
    sum += w;
    w *= m_p.skip;
  }
  for (auto j = start; j < end; ++j)
    m_prediction[j] *= (1.0 - m_p.jumpAfterPause) / sum;
  const auto [jumpBegin, jumpEnd] = JumpSpan();
  m_begin = std::min(start, jumpBegin);
  m_end = std::max(end, jumpEnd);
  AddJumps(m_p.jumpAfterPause);
  std::fill(m_onsetMs.begin() + m_begin, m_onsetMs.begin() + m_end, ms);
}

std::pair<size_t, size_t> PerformanceFollower::JumpSpan() const
{
  const double from = m_events[m_followed].tick;
  const auto reach = m_p.jumpCutoff * ticksPerQuarter;
  const auto begin = std::lower_bound(
      m_events.begin(), m_events.end(), from - reach * m_p.backJumpScale,
      [](const ScoreEvent &event, double tick) { return event.tick < tick; });
  const auto end = std::upper_bound(
      m_events.begin(), m_events.end(), from + reach * m_p.forwardJumpScale,
      [](double tick, const ScoreEvent &event) { return tick < event.tick; });
  return {static_cast<size_t>(begin - m_events.begin()),
          static_cast<size_t>(end - m_events.begin())};
}

void PerformanceFollower::AddJumps(double mass)
{
  // From the followed event rather than from each event: an approximation,
  // the posterior being mostly concentrated, that keeps a note's cost to the
  // span jumps reach.
  const auto from = m_events[m_followed].tick;
  const auto [begin, end] = JumpSpan();
  auto sum = 0.0;
  for (auto j = begin; j < end; ++j)
  {
    const auto quarters = (m_events[j].tick - from) / ticksPerQuarter;
    const auto scale =
        quarters < 0 ? m_p.backJumpScale : m_p.forwardJumpScale;
    m_jumpWeights[j] = std::exp(-std::abs(quarters) / scale);
    sum += m_jumpWeights[j];
  }
  for (auto j = begin; j < end; ++j)
    m_prediction[j] += mass * m_jumpWeights[j] / sum;
}

void PerformanceFollower::UpdateTempo(size_t event, double ms)
{
  if (!m_hasAnchor || event < m_anchorEvent)
  {
    m_hasAnchor = true;
    m_anchorEvent = event;
    m_anchorMs = ms;
    return;
  }
  const auto spanMs = ms - m_anchorMs;
  if (spanMs < m_p.minTempoSpanMs)
    // Keep measuring from the same anchor.
    return;
  const auto ticks = m_events[event].tick - m_events[m_anchorEvent].tick;
  const auto ratio = ticks / spanMs / m_ticksPerMs;
  if (ratio > 1.0 / maxTempoStep && ratio < maxTempoStep)
  {
    m_ticksPerMs *= std::pow(ratio, m_p.tempoSmoothing);
    m_ticksPerMs = std::clamp(m_ticksPerMs, m_nominalTicksPerMs * minTempoRatio,
                              m_nominalTicksPerMs * maxTempoRatio);
  }
  m_anchorEvent = event;
  m_anchorMs = ms;
}
} // namespace dgk
