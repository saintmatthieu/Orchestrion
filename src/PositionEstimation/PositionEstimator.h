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
#pragma once

#include "PositionSmoother.h"
#include "PositionTracker.h"

#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace dgk
{
//! Where each hand of a performance has got to in the score, and how well it
//! kept its own time getting there.
//!
//! One PositionTracker and one PositionSmoother per hand (= staff; the voices
//! on a staff are played by the same gestures, so they share one estimate),
//! fed the onsets that hand plays. The tracker answers "where is this hand
//! *now*" between onsets — causal, so usable to drive something in real time.
//! The smoother answers the retrospective question — "given what came after,
//! where should that onset have fallen" — whose residuals are the timing
//! errors.
//!
//! Positions are **playback-unrolled ticks** (uticks): unlike score ticks
//! they run monotonically through repeats, voltas and jumps, so an estimate
//! carries straight across them. Tempo is just the derivative, reported in
//! BPM for readouts.
//!
//! Deliberately free of Qt and of any notion of what the estimate is *for*:
//! no clock of its own (callers pass the time), no timers, no view, no score
//! access, no policy about which hands matter.
class PositionEstimator
{
public:
  //! Verdict on one onset's timing, measured against its hand's *smoothed*
  //! curve. The curve bends with the performer, so a smooth tempo shape
  //! (rubato, a ritardando) is not an error; only deviation from the
  //! performer's own smooth curve is. Since the curve is re-fitted on every
  //! onset, the same onset — identified by \p tMs — is re-judged as it gains
  //! hindsight, until it leaves the smoothing window.
  struct Judgment
  {
    double tMs;     //!< the onset's time, as given by the caller: its identity
    double errorMs; //!< signed arrival error: − = early, + = late
    //! How far the fitted smooth timeline displaces this note from the
    //! window's constant-tempo reference (+ = later), in ticks, and the
    //! actual arrival's additional displacement from the fitted one.
    double warpTicks = 0.0;
    double errorTicks = 0.0;
    //! The same fitted displacement in milliseconds (a note's total
    //! deviation from the reference is warpMs + errorMs).
    double warpMs = 0.0;
    //! The fitted tempo at this onset, in BPM; re-fitted like the rest.
    double bpm = 0.0;
  };

  //! What one batch of onsets yields.
  struct Feedback
  {
    //! Per hand that struck in this batch, the time stamp its judgments
    //! carry. Present from the very first onset, before the curve warms up,
    //! so a display can create the onset's marks up front and fill their
    //! values in as judgments arrive (retroactively for the first onsets).
    std::map<int /*staff*/, double /*tMs*/> onsetTMs;
    //! Per hand that struck, the (re-)judgments of *all* its onsets still in
    //! the smoothing window, newest last — empty while the curve is warming
    //! up (at the start, or after a stop).
    std::map<int /*staff*/, std::vector<Judgment>> judgments;
  };

  //! A point of a hand's smoothed tempo curve, for plotting it.
  struct CurvePoint
  {
    double tMs;
    double bpm;
  };

  //! \p memory is the smoothing memory γ ∈ (0,1) of the hands' curves: how
  //! stiff the fitted tempo is (higher = smoother, less tolerant of
  //! short-term deviation).
  explicit PositionEstimator(double memory = 0.6);

  //! Applies to hands created from now on; hands already tracking keep theirs
  //! (a take is fitted with one γ throughout).
  void setMemory(double memory);

  //! Feed the onsets played at \p nowMs, as staff → utick. Hands not
  //! mentioned are untouched.
  Feedback onOnsets(double nowMs,
                    const std::map<int /*staff*/, double /*utick*/> &onsets);

  //! Let time pass without onsets: hands whose next note is overdue coast to
  //! a stop rather than extrapolating forever.
  void heartbeat(double nowMs);

  //! Whether \p staff has enough onsets for its estimate to mean anything.
  bool ready(int staff) const;
  //! The hand's estimated utick at \p nowMs — the live, causal estimate.
  std::optional<double> tickAt(int staff, double nowMs) const;
  //! The hand's live tempo in BPM (the causal estimate's derivative).
  std::optional<double> bpm(int staff) const;
  //! Whether the hand's next onset is overdue, i.e. it is coasting to a stop.
  bool isCoasting(int staff) const;
  //! The hand's smoothed tempo curve over its window, for plotting.
  std::vector<CurvePoint> smoothedBpmCurve(int staff) const;

  //! How far apart two hands are, in milliseconds of music, at a recent
  //! instant where both smoothed curves have settled (+ = \p leadingStaff is
  //! ahead). Absent unless both hands are playing and warmed up. The caller
  //! chooses the pair: an accompaniment played by the machine, say, has
  //! nothing to say about a performer's synchronisation.
  std::optional<Judgment> asynchrony(int leadingStaff, int laggingStaff,
                                     double nowMs) const;

  //! Forget every hand (a new take, a jump, a new score).
  void reset();

  //! Re-fit a whole take offline with a different smoothing memory γ: feed
  //! its (time ms, utick) observations, in order, through an unbounded
  //! smoother and return the re-judged onsets.
  static std::vector<Judgment>
  refitTake(const std::vector<std::pair<double, double>> &observations,
            double memory);

private:
  struct Hand
  {
    explicit Hand(double memory) : tracker{memory}, smoother{memory} {}
    PositionTracker tracker;
    PositionSmoother smoother;
    double lastUtick = 0.0;
    bool hasOnset = false;
  };

  const Hand *find(int staff) const;

  std::map<int /*staff*/, Hand> _hands;
  double _memory;
};
} // namespace dgk
