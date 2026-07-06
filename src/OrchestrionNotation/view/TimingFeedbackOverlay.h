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

#include "TempoFollower.h"

#include <QElapsedTimer>
#include <QRectF>
#include <QString>
#include <QTimer>

#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <vector>

class QPainter;

namespace dgk
{
//! Paints the timing-judgment feedback. The judgments are *retrospective* —
//! each onset's error is measured against the hand's smoothed tempo spline
//! and re-evaluated as later onsets refine it (see TempoFollower::Judgment)
//! — so everything shown here is live in both directions: new marks appear,
//! and recent marks settle as hindsight arrives.
//!
//! - per judged onset, a transient *gauge* near the struck notes — above the
//!   staff for the right hand, below it for the left, always kept clear of
//!   the staff lines: a vertical ruler spanning a fixed ± error range, with a
//!   centre mark (dead on time) and a marker where the onset landed relative
//!   to the spline — high = early (rushing), low = late (dragging); an
//!   arrowhead past the ruler's end = outlier. The marker's colour grades
//!   continuously from green (dead on) to red (outlier), and the marker moves
//!   while its gauge shows if the onset gets re-judged;
//! - a short-term *box plot* of the recent errors (recency-weighted, over
//!   roughly the last minute): median line, quartile box, Tukey whiskers and
//!   outlier dots, docked to the bottom-right of the viewport at a constant
//!   physical size, re-computed from the revised errors.
//!
//! Owns its fade timer (mirroring HighlightFader): the owner feeds it
//! judgments and paints it on top of the notation with the painter in
//! score-logical coordinates.
class TimingFeedbackOverlay
{
public:
  //! \p requestRepaint is invoked whenever the fading gauges change.
  explicit TimingFeedbackOverlay(std::function<void()> requestRepaint);

  TimingFeedbackOverlay(const TimingFeedbackOverlay &) = delete;
  TimingFeedbackOverlay &operator=(const TimingFeedbackOverlay &) = delete;

  //! Begin showing a gauge for \p staff's newest onset, near \p noteRect (the
  //! struck notes' hugging box, score-logical). \p onsetTMs is the onset's
  //! identity (TempoFollower's clock, as carried by its judgments), by which
  //! later updateJudgments() calls revise the marker; \p errorMs is the
  //! initial error. The gauge goes below the staff when \p belowStaff (the
  //! left hand), else above; \p staffEdgeY is the staff's facing edge (bottom
  //! resp. top line, page-logical y), which the ruler keeps clear of.
  void addGauge(int staff, double onsetTMs, const QRectF &noteRect,
                double spatium, double errorMs, bool belowStaff,
                double staffEdgeY);

  //! The (re-)judgments of a hand's onsets still in its smoothing window:
  //! upsert the stats samples (keyed by onset identity) and move the
  //! marker of any of its gauges still showing.
  void updateJudgments(int staff,
                       const std::vector<TempoFollower::Judgment> &window);

  //! One hand-asynchrony sample (see TempoFollower::Feedback::handSync): fed
  //! into the sync component of the score. Never arrives for single-staff
  //! scores — or while the sync component is disabled — and the score then
  //! has no sync part.
  void addSyncSample(double tMs, double errorMs);

  //! Drop the sync samples (the component was toggled off mid-take, and its
  //! stale samples shouldn't linger in the verdict).
  void clearSyncStats();

  //! When persistent, gauges don't fade: every judged onset keeps its mark on
  //! the page (a repeat pass stacks its marks beyond the earlier ones) until
  //! the stats reset — so a whole take can be reviewed after playing it.
  //! Turning persistence off fades the accumulated marks out from now.
  void setPersistent(bool persistent);

  //! The verdict of the gauge under \p logicalPos, for a tooltip (e.g.
  //! "23 ms late"); empty if none is hit.
  QString gaugeInfoAt(const QPointF &logicalPos) const;

  //! Forget everything, stats included — any interruption of play (stop,
  //! a click or swipe, a position jump, a new score) starts the stats over.
  void reset();

  //! The final score of the whole take (everything since the last reset(),
  //! unweighted): the average of the available component scores — tempo
  //! smoothness, and hand sync when the piece has two hands. Each component
  //! is 100·e^(−m/m₀), m the 80th percentile of its |error| — robust like
  //! the box plot, but centred on zero so both bias and spread cost points.
  //! The score shown above the box plot is the recency-weighted sibling.
  //! Empty while there are no samples.
  std::optional<int> takeFinalScore() const;

  //! The take's component scores for display next to the final score, e.g.
  //! "tempo 87 · sync 92" (no sync part for single-staff scores); empty while
  //! there are no samples.
  QString takeScoreBreakdown() const;

  //! Paint gauges and box plot. The painter must be in score-logical
  //! coordinates; \p viewport is the visible logical rect and \p scaling the
  //! zoom (physical px per logical unit), used to keep the box-plot HUD a
  //! constant physical size.
  void paint(QPainter &painter, const QRectF &viewport, double scaling) const;

private:
  //! Timer tick: drop fully-faded gauges, stop when none remain, repaint.
  void advance();

  struct Gauge
  {
    int staff;
    double onsetTMs; // the onset's identity, for judgment revisions
    double x;        // logical x, centred on the struck notes
    double centerY;  // logical y of the ruler's zero mark
    double spatium;  // score staff-space unit: sizes the ruler
    double errorMs;  // latest verdict; revised while the gauge shows
    qint64 startMs;
  };
  void paintGauge(QPainter &painter, const Gauge &gauge,
                  double opacity) const;
  void paintBoxPlot(QPainter &painter, const QRectF &viewport,
                    double scaling) const;
  void pruneSamples();
  //! The score metrics (80th percentile of |error|, ms) over the recent
  //! window, recency-weighted — what the live score display shows.
  std::optional<double> recentAbsErrorQuantile() const;
  std::optional<double> recentSyncAbsErrorQuantile() const;
  //! Their whole-take, unweighted siblings — the final verdict.
  std::optional<double> takeAbsErrorQuantile() const;
  std::optional<double> takeSyncAbsErrorQuantile() const;

  const std::function<void()> _requestRepaint;
  QElapsedTimer _clock; // free-running; timestamps gauges and samples
  QTimer _timer;        // ~60 Hz while gauges are fading

  std::deque<Gauge> _gauges;
  bool _persistent = false;

  //! The recent error samples backing the box plot, keyed per staff by onset
  //! identity so revised judgments overwrite in place. The statistics are
  //! computed (weighted by recentness of arrival) at paint time; entries are
  //! pruned past the window.
  struct Sample
  {
    double errorMs;
    qint64 arrivalMs;
  };
  std::map<int /*staff*/, std::map<double /*onsetTMs*/, Sample>> _samples;

  //! The whole take's (revised) errors, never pruned, backing the take's
  //! tempo-smoothness component.
  std::map<int /*staff*/, std::map<double /*onsetTMs*/, double /*errorMs*/>>
      _takeSamples;

  //! Hand-asynchrony samples: the rolling window (recency-weighted, like
  //! _samples) and the whole take's, backing the sync component.
  std::deque<Sample> _syncSamples;
  std::vector<double> _takeSyncErrors;
};
} // namespace dgk
