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
   * Whether a bar begins here: where a performer who restarts most likely
   * restarts from.
   */
  bool startsBar = false;
  /**
   * Whether the event's pitches are struck over and over until the next
   * event, as many times as the performer likes: a trill.
   */
  bool trill = false;
};

/**
 * The tuning of the PerformanceFollower's model. Probabilities need not be
 * normalized: only their ratios matter.
 */
struct PerformanceFollowerParameters
{
  /** Emission of a pitch of the event. */
  double match = 1.0;
  /** Emission of a pitch a tone or less away from one of the event's. */
  double nearMiss = 0.05;
  /** Emission of a pitch an octave away from one of the event's. */
  double octave = 0.03;
  /** Emission of any other pitch. */
  double noise = 0.005;

  /**
   * Staying on the event for another of its pitches, decaying with the time
   * since the previous note as exp(-dt / chordSpreadMs).
   */
  double chordMember = 1.0;
  double chordSpreadMs = 40.0;
  /**
   * Staying on a trill for another of its notes, until its written length is
   * up: then decaying as 1 / (1 + (elapsed / length)^trillEndSharpness).
   */
  double trillNote = 1.0;
  double trillEndSharpness = 8.0;
  /** Staying on the event for an extra note, whenever. */
  double insertion = 0.05;
  /** Moving on to the next event. */
  double advance = 1.0;
  /** Each event skipped (missed) multiplies the advance by this. */
  double skip = 0.1;
  /** Farthest local move, in events; beyond that is a jump. */
  int maxAdvance = 6;
  /**
   * Spread of the tempo deviation the timing of an advance tolerates: the
   * standard deviation of log(score time / expected score time).
   */
  double timingSigma = 0.45;
  /**
   * Added to both sides of the timing ratio, in milliseconds at the current
   * tempo, so that near-simultaneous events don't give extreme ratios.
   */
  double timingFloorMs = 30.0;
  /**
   * A gap after which the performer is considered to have paused: timing
   * tells nothing about the next move, and jumps are likelier.
   */
  double pauseMs = 1500.0;

  /** Jumping anywhere in the score, per note. */
  double jump = 1e-4;
  /** The same, after a pause. */
  double jumpAfterPause = 0.05;
  /** How much likelier a jump lands on a bar start than elsewhere. */
  double barStartJumpWeight = 20.0;
  /**
   * The share of jumps landing near where the performer was followed, rather
   * than anywhere: performers mostly go back a little to try again, or skip
   * a little ahead. Resolves restarts in repeated passages.
   */
  double nearJumpShare = 0.9;
  /**
   * The likelihood of a near jump decays exponentially with how far it
   * goes, in milliseconds at the current tempo: slowly backward, fast
   * forward.
   */
  double backJumpScaleMs = 10000.0;
  double forwardJumpScaleMs = 2000.0;

  /** The posterior an event needs for FollowedEvent() to move to it. */
  double followThreshold = 0.5;
  /**
   * The tempo is measured over spans of at least this long, so that fast
   * notes (ornaments, which are played at their own speed) don't skew it.
   */
  double minTempoSpanMs = 250.0;
  /** How much of each tempo measurement goes into the estimate. */
  double tempoSmoothing = 0.3;
};

/**
 * Follows a performance of a score in real time: from the pitches and times
 * of the note-ons played, estimates where in the score the performer is,
 * tolerating wrong, missed and extra notes, tempo changes, and restarts from
 * anywhere.
 *
 * A hidden Markov model whose states are the score's events, filtered with
 * the forward algorithm. Each note-on either belongs to the current event (a
 * chord's notes come in close together) or moves on, preferably to the next
 * event, at a time consistent with the current tempo. Jumps anywhere are
 * spread independently of where they come from (weighted toward bar starts
 * and toward the followed position) so that a note costs O(events) whatever
 * the score's length.
 *
 * The approach follows:
 * - E. Nakamura, T. Nakamura, Y. Saito, N. Ono, S. Sagayama, "Outer-Product
 *   Hidden Markov Model and Polyphonic MIDI Score Following", Journal of New
 *   Music Research 43(2):183-201, 2014 (arbitrary repeats and skips, their
 *   efficient computation, performance errors);
 * - E. Nakamura, P. Cuvillier, A. Cont, N. Ono, S. Sagayama, "Autoregressive
 *   Hidden Semi-Markov Model of Symbolic Music Performance for Score
 *   Following", ISMIR 2015 (the timing of chords and of moves between
 *   events);
 * - R. B. Dannenberg, "An On-Line Algorithm for Real-Time Accompaniment",
 *   ICMC 1984, which started the field.
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
  PerformanceFollower(std::vector<ScoreEvent> events, double nominalTicksPerMs,
                      PerformanceFollowerParameters parameters = {});

  /**
   * Expect the next note at `event` (the performer is about to start there,
   * e.g. after a click in the score). Keeps the tempo estimate.
   */
  void Reset(size_t event);

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
  void AddJumps(double mass);
  void UpdateTempo(size_t event, double ms);

  const std::vector<ScoreEvent> m_events;
  const PerformanceFollowerParameters m_p;
  /** Where a jump anywhere lands, normalized. */
  std::vector<double> m_jumpWeights;
  std::vector<double> m_nearJumpWeights;
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
  double m_lastMs = 0.0;
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
