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
#include <limits>

namespace dgk
{
namespace
{
constexpr double kMinTempoRatio = 0.25;
constexpr double kMaxTempoRatio = 4.0;
/**
 * A tempo measurement this far off the estimate is taken for a
 * misalignment rather than a tempo change.
 */
constexpr double kMaxTempoStep = 2.0;
/** States this much less likely than the likeliest aren't propagated. */
constexpr double kPruning = 1e-12;
/** MuseScore's. */
constexpr double kTicksPerQuarter = 480.0;

// The tuning of the model. Probabilities need not be normalized: only their
// ratios matter.

/** Emission of a pitch of the event. */
constexpr double kMatch = 1.0;
/** Emission of a pitch a tone or less away from one of the event's. */
constexpr double kNearMiss = 0.05;
/** Emission of a pitch an octave away from one of the event's. */
constexpr double kOctave = 0.03;
/** Emission of any other pitch. */
constexpr double kNoise = 0.005;

/**
 * Staying on the event for another of its pitches, while it has some not
 * played yet, decaying with the time since the event began as
 * exp(-dt / kChordSpreadMs).
 */
constexpr double kChordMember = 1.0;
constexpr double kChordSpreadMs = 40.0;
/**
 * Staying on a trill for another of its notes, until its written length is
 * up: then decaying as 1 / (1 + (elapsed / length)^kTrillEndSharpness).
 */
constexpr double kTrillNote = 1.0;
constexpr double kTrillEndSharpness = 8.0;
/** Staying on the event for an extra note, whenever. */
constexpr double kInsertion = 0.05;
/** Moving on to the next event. */
constexpr double kAdvance = 1.0;
/** Each event skipped (missed) multiplies the advance by this. */
constexpr double kSkip = 0.1;
/** Farthest local move, in events; beyond that is a jump. */
constexpr int kMaxAdvance = 6;
/**
 * Spread of how early a move can come: the standard deviation of
 * log(score distance covered / score distance of the move). Lateness is
 * tolerated whatever it is: learners stop to think.
 */
constexpr double kTimingSigma = 0.45;
/**
 * Added to both sides of the timing ratio, in milliseconds at the current
 * tempo, so that near-simultaneous events don't give extreme ratios.
 */
constexpr double kTimingFloorMs = 30.0;
/**
 * The least an early move weighs, however early: the tempo may be off — learnt
 * from a passage played slowly for its difficulty, say — and timing then only
 * chooses between moves the pitch can't tell apart.
 */
constexpr double kEarlyFloor = 0.03;

/**
 * Jumping, per note: performers go back a little to try again, or skip a
 * little ahead. Farther, they click in the score.
 */
constexpr double kJump = 1e-4;
/**
 * The same for the first note after a reset — a click where the performer is
 * about to start —, spread around where they clicked.
 */
constexpr double kJumpAtReset = 0.05;
/**
 * The likelihood of a jump decays as exp(-distance / scale) with how far
 * it goes from where the performer was followed, in quarter notes:
 * slowly backward, fast forward.
 */
constexpr double kBackJumpScale = 4.0;
constexpr double kForwardJumpScale = 1.0;
/**
 * Beyond this many scales, no jump lands — unless what the performer sees
 * is known (see PerformanceFollower::SetVisibleTicks).
 */
constexpr double kJumpCutoff = 5.0;

/** The posterior an event needs for FollowedEvent() to move to it. */
constexpr double kFollowThreshold = 0.5;
/**
 * The tempo is measured over spans of at least this long, so that fast
 * notes (ornaments, which are played at their own speed) don't skew it.
 */
constexpr double kMinTempoSpanMs = 250.0;
/** How much of each tempo measurement goes into the estimate. */
constexpr double kTempoSmoothing = 0.3;
} // namespace

PerformanceFollower::PerformanceFollower(std::vector<ScoreEvent> events,
                                         double nominalTicksPerMs)
    : m_events{std::move(events)}, m_ticksPerMs{nominalTicksPerMs},
      m_nominalTicksPerMs{nominalTicksPerMs}
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
  m_notesPlayed.assign(m_events.size(), 0);
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

void PerformanceFollower::SetVisibleTicks(int begin, int end)
{
  std::vector<std::pair<size_t, size_t>> runs;
  for (size_t j = 0; j < m_events.size(); ++j)
  {
    const auto tick = m_events[j].writtenTick;
    if (tick < begin || tick >= end)
      continue;
    if (!runs.empty() && runs.back().second == j)
      ++runs.back().second;
    else
      runs.emplace_back(j, j + 1);
  }
  m_visibleRuns = std::move(runs);
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
  {
    m_posterior[j] /= sum;
    // Wherever the performer is, this note was played there — but a wrong
    // one leaves the right one still to play.
    const auto &pitches = m_events[j].pitches;
    if (std::find(pitches.begin(), pitches.end(), pitch) != pitches.end())
      ++m_notesPlayed[j];
  }

  const auto best =
      static_cast<size_t>(std::max_element(m_posterior.begin() + m_begin,
                                           m_posterior.begin() + m_end) -
                          m_posterior.begin());
  const Estimate estimate{best, m_posterior[best]};

  if (estimate.probability >= kFollowThreshold)
  {
    m_followed = best;
    if (best != m_lastBest || !m_hasAnchor)
      UpdateTempo(best, ms);
    m_lastBest = best;
  }

  m_fresh = false;
  return estimate;
}

double PerformanceFollower::Emission(int pitch, const ScoreEvent &event) const
{
  auto e = kNoise;
  for (const auto p : event.pitches)
  {
    const auto distance = std::abs(pitch - p);
    if (distance == 0)
      return kMatch;
    if (distance <= 2)
      e = std::max(e, kNearMiss);
    else if (distance == 12)
      e = std::max(e, kOctave);
  }
  return e;
}

double PerformanceFollower::Timing(int ticks, double dtMs) const
{
  const auto floor = kTimingFloorMs * m_ticksPerMs;
  // How far the performer went at the current tempo, against how far the
  // score says.
  const auto played = std::max(dtMs, 0.0) * m_ticksPerMs;
  if (played >= ticks)
    // Late: a learner thinking, most likely, whom the pitch tells apart from
    // one who skipped.
    return 1.0;
  const auto r = std::log((played + floor) / (ticks + floor)) / kTimingSigma;
  return std::max(std::exp(-0.5 * r * r), kEarlyFloor);
}

double PerformanceFollower::Stay(size_t event, double sinceOnsetMs) const
{
  // Whichever of them: the emission doesn't share its weight among the notes
  // remaining, so neither does staying.
  const bool remaining = m_notesPlayed[event] <
                         static_cast<int>(m_events[event].pitches.size());
  const auto chord =
      remaining ? kChordMember * std::exp(-sinceOnsetMs / kChordSpreadMs)
                : 0.0;
  if (!m_events[event].trill)
    return chord;
  if (event + 1 == m_events.size())
    return std::max(chord, kTrillNote);
  const auto length = m_events[event + 1].tick - m_events[event].tick;
  const auto elapsed = sinceOnsetMs * m_ticksPerMs / length;
  return std::max(chord,
                  kTrillNote / (1.0 + std::pow(elapsed, kTrillEndSharpness)));
}

void PerformanceFollower::Predict(double ms)
{
  const auto n = m_events.size();
  const auto maxPosterior = *std::max_element(m_posterior.begin() + m_begin,
                                              m_posterior.begin() + m_end);

  std::fill(m_prediction.begin() + m_begin, m_prediction.begin() + m_end, 0.0);
  std::fill(m_stayed.begin() + m_begin, m_stayed.begin() + m_end, 0.0);
  auto first = m_end;
  auto last = m_begin;
  std::vector<double> w(kMaxAdvance + 1);
  for (auto i = m_begin; i < m_end; ++i)
  {
    const auto a = m_posterior[i];
    if (a < kPruning * maxPosterior)
      continue;
    first = std::min(first, i);
    last = i;
    // Timed from the event's onset rather than from the previous note, so
    // that an event of many notes (a trill) is left when its time is up.
    const auto sinceOnset = ms - m_onsetMs[i];
    w[0] = kInsertion + Stay(i, sinceOnset);
    auto sum = w[0];
    auto advance = kAdvance;
    size_t k = 1;
    for (; k < w.size() && i + k < n; ++k)
    {
      const auto ticks = m_events[i + k].tick - m_events[i].tick;
      w[k] = advance * Timing(ticks, sinceOnset);
      sum += w[k];
      advance *= kSkip;
    }
    const auto local = a * (1.0 - kJump) / sum;
    m_stayed[i] = local * w[0];
    for (size_t l = 0; l < k; ++l)
      m_prediction[i + l] += local * w[l];
  }
  std::fill(m_posterior.begin() + m_begin, m_posterior.begin() + m_end, 0.0);
  const auto [jumpBegin, jumpEnd] = JumpSpan();
  m_begin = std::min(first, jumpBegin);
  m_end = std::max(std::min(n, last + kMaxAdvance + 1), jumpEnd);
  AddJumps(kJump);

  // An event mostly arrived at, rather than stayed on, begins now.
  for (auto j = m_begin; j < m_end; ++j)
    if (m_stayed[j] < 0.5 * m_prediction[j])
    {
      m_onsetMs[j] = ms;
      m_notesPlayed[j] = 0;
    }
}

void PerformanceFollower::PredictFrom(size_t start, double ms)
{
  const auto n = m_events.size();
  std::fill(m_prediction.begin() + m_begin, m_prediction.begin() + m_end, 0.0);
  std::fill(m_stayed.begin() + m_begin, m_stayed.begin() + m_end, 0.0);
  std::fill(m_posterior.begin() + m_begin, m_posterior.begin() + m_end, 0.0);
  const auto end = std::min(n, start + kMaxAdvance + 1);
  auto w = 1.0;
  auto sum = 0.0;
  for (auto j = start; j < end; ++j)
  {
    m_prediction[j] = w;
    sum += w;
    w *= kSkip;
  }
  for (auto j = start; j < end; ++j)
    m_prediction[j] *= (1.0 - kJumpAtReset) / sum;
  const auto [jumpBegin, jumpEnd] = JumpSpan();
  m_begin = std::min(start, jumpBegin);
  m_end = std::max(end, jumpEnd);
  AddJumps(kJumpAtReset);
  std::fill(m_onsetMs.begin() + m_begin, m_onsetMs.begin() + m_end, ms);
  std::fill(m_notesPlayed.begin() + m_begin, m_notesPlayed.begin() + m_end, 0);
}

std::pair<size_t, size_t> PerformanceFollower::JumpSpan() const
{
  if (m_visibleRuns)
  {
    const auto &runs = *m_visibleRuns;
    if (runs.empty())
      return {m_followed, m_followed + 1};
    // The first run that doesn't end before the followed event, or the one
    // before it if that is nearer.
    auto run = std::partition_point(runs.begin(), runs.end(),
                                    [this](const std::pair<size_t, size_t> &r)
                                    { return r.second <= m_followed; });
    if (run == runs.end() ||
        (run->first > m_followed && run != runs.begin() &&
         m_followed - (run - 1)->second < run->first - m_followed))
      --run;
    return *run;
  }
  const double from = m_events[m_followed].tick;
  const auto reach = kJumpCutoff * kTicksPerQuarter;
  const auto begin = std::lower_bound(
      m_events.begin(), m_events.end(), from - reach * kBackJumpScale,
      [](const ScoreEvent &event, double tick) { return event.tick < tick; });
  const auto end = std::upper_bound(
      m_events.begin(), m_events.end(), from + reach * kForwardJumpScale,
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
  // Log weights first, the largest taken out: the span may be far from the
  // followed event, where the weights themselves would all be 0.
  auto largest = -std::numeric_limits<double>::infinity();
  for (auto j = begin; j < end; ++j)
  {
    const auto quarters = (m_events[j].tick - from) / kTicksPerQuarter;
    const auto scale = quarters < 0 ? kBackJumpScale : kForwardJumpScale;
    m_jumpWeights[j] = -std::abs(quarters) / scale;
    largest = std::max(largest, m_jumpWeights[j]);
  }
  auto sum = 0.0;
  for (auto j = begin; j < end; ++j)
  {
    m_jumpWeights[j] = std::exp(m_jumpWeights[j] - largest);
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
  if (spanMs < kMinTempoSpanMs)
    // Keep measuring from the same anchor.
    return;
  const auto ticks = m_events[event].tick - m_events[m_anchorEvent].tick;
  const auto ratio = ticks / spanMs / m_ticksPerMs;
  if (ratio > 1.0 / kMaxTempoStep && ratio < kMaxTempoStep)
  {
    m_ticksPerMs *= std::pow(ratio, kTempoSmoothing);
    m_ticksPerMs =
        std::clamp(m_ticksPerMs, m_nominalTicksPerMs * kMinTempoRatio,
                   m_nominalTicksPerMs * kMaxTempoRatio);
  }
  m_anchorEvent = event;
  m_anchorMs = ms;
}
} // namespace dgk
