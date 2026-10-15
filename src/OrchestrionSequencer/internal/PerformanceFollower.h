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
#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace dgk
{
/**
 * One onset of the followed part: the pitches struck at a tick. Ornaments
 * are written out beforehand, their notes being events of their own.
 */
struct ScoreEvent
{
  int tick = 0;
  std::vector<int> pitches;
  /**
   * Whether the event's pitches are struck over and over until the next
   * event, as many times as the performer likes: a trill.
   */
  bool trill = false;
  /** The tick without repeats: where the event is written in the score. */
  int writtenTick = 0;
};

/**
 * Follows a performance of a score in real time: from the pitches and times
 * of the note-ons played, estimates where in the score the performer is,
 * tolerating wrong, missed and extra notes, tempo changes, and restarts
 * nearby.
 *
 * A hidden Markov model whose states are the score's events, filtered with
 * the forward algorithm. Each note-on either belongs to the current event (a
 * chord's notes come in close together) or moves on, preferably to the next
 * event, at a time consistent with the current tempo. Jumps are spread
 * around the followed position, independently of where they come from, and
 * only the events near it hold any probability, so that a note's cost
 * doesn't grow with the score's length.
 *
 * The approach follows:
 * - E. Nakamura, T. Nakamura, Y. Saito, N. Ono, S. Sagayama, "Outer-Product
 *   Hidden Markov Model and Polyphonic MIDI Score Following", Journal of New
 *   Music Research 43(2):183-201, 2014 (arbitrary repeats and skips, their
 *   efficient computation, performance errors);
 * - E. Nakamura, P. Cuvillier, A. Cont, N. Ono, S. Sagayama, "Autoregressive
 *   Hidden Semi-Markov Model of Symbolic Music Performance for Score
 *   Following", ISMIR 2015 (timing in the transitions rather than in the
 *   emissions);
 * - R. B. Dannenberg, "An On-Line Algorithm for Real-Time Accompaniment",
 *   ICMC 1984, which started the field.
 *
 * The timing model itself is ours, not the papers': a Gaussian on the log of
 * the ratio of the score distance covered at the current tempo to the score
 * distance of the move, with a floor against near-simultaneous events, and a
 * hand-picked spread. The papers model time between notes linearly instead,
 * with a mixture of noises for motor noise and timing errors (see also
 * E. Nakamura, N. Ono, S. Sagayama, K. Watanabe, "A Stochastic Temporal
 * Model of Polyphonic MIDI Performance with Ornaments", Journal of New Music
 * Research 44(4), 2015, §3.3-3.4).
 */
class PerformanceFollower
{
public:
  struct Estimate
  {
    /** The most likely event. */
    size_t event = 0;
    /** Its posterior probability. */
    double probability = 0.0;
  };

  /**
   * `events` must be non-empty and sorted by strictly increasing tick;
   * `nominalTicksPerMs` is the score's tempo, the initial estimate of the
   * performer's.
   */
  PerformanceFollower(std::vector<ScoreEvent> events, double nominalTicksPerMs);

  /**
   * Expect the next note at `event` (the performer is about to start there,
   * e.g. after a click in the score). Keeps the tempo estimate.
   */
  void Reset(size_t event);

  /**
   * The part of the score the performer sees, in written ticks (see
   * ScoreEvent::writtenTick) as [begin, end): jumps land there only, where it
   * is played nearest to the followed event if the score repeats it.
   */
  void SetVisibleTicks(int begin, int end);

  /** A note-on of `pitch` at time `ms` (monotonic, any origin). */
  Estimate OnNoteOn(int pitch, double ms);

  /**
   * The event the performer is followed at: the most likely one, but moved
   * to only when likely enough, so that a doubtful note doesn't make it
   * flicker.
   */
  size_t FollowedEvent() const { return m_followed; }

  /** The estimated tempo of the performance. */
  double TicksPerMs() const { return m_ticksPerMs; }
  /**
   * Takes the tempo of the performance from elsewhere — an estimate of it fed
   * by more than the notes followed — for the next note's timing.
   */
  void SetTicksPerMs(double ticksPerMs) { m_ticksPerMs = ticksPerMs; }

  const std::vector<ScoreEvent> &Events() const { return m_events; }

private:
  double Emission(int pitch, const ScoreEvent &) const;
  double Timing(int ticks, double dtMs) const;
  /**
   * The weight of staying on `event` for another note, `sinceOnsetMs` after
   * it began (the extra-note weight aside).
   */
  double Stay(size_t event, double sinceOnsetMs) const;
  void Predict(double ms);
  void PredictFrom(size_t start, double ms);
  /** The events a jump can land on, as [begin, end). */
  std::pair<size_t, size_t> JumpSpan() const;
  void AddJumps(double mass);
  void UpdateTempo(size_t event, double ms);

  const std::vector<ScoreEvent> m_events;
  std::vector<double> m_jumpWeights;
  /**
   * The runs of consecutive events the performer sees, as [begin, end), if
   * that is known.
   */
  std::optional<std::vector<std::pair<size_t, size_t>>> m_visibleRuns;
  /** The events holding probability: it is zero outside. */
  size_t m_begin = 0;
  size_t m_end = 0;
  /** The posterior over events after the last note. */
  std::vector<double> m_posterior;
  std::vector<double> m_prediction;
  /** The share of each event's prediction that stayed on it. */
  std::vector<double> m_stayed;
  /** When each event, if the performer is at it, began. */
  std::vector<double> m_onsetMs;
  /** Whether no note was played since the last reset. */
  bool m_fresh = true;
  size_t m_start = 0;
  size_t m_followed = 0;
  double m_ticksPerMs;
  const double m_nominalTicksPerMs;
  /** The event and time the current tempo measurement started at. */
  bool m_hasAnchor = false;
  size_t m_anchorEvent = 0;
  double m_anchorMs = 0.0;
  size_t m_lastBest = 0;
};
} // namespace dgk
