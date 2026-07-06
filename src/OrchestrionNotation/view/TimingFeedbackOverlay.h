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

#include <QElapsedTimer>
#include <QRectF>
#include <QTimer>

#include <deque>
#include <functional>

class QPainter;

namespace dgk
{
//! Paints the timing-judgment feedback:
//!
//! - per judged onset, a transient *gauge* near the struck notes — above the
//!   staff for the right hand, below it for the left, always kept clear of
//!   the staff lines: a vertical ruler spanning a fixed ± error range, with a
//!   centre mark (dead on time) and a marker where the onset actually landed
//!   — high = early (rushing), low = late (dragging); an arrowhead past the
//!   ruler's end = outlier. The marker's colour grades continuously from
//!   green (dead on) to red (outlier);
//! - a short-term *histogram* of the recent error samples (recency-weighted,
//!   over roughly the last minute), docked to the bottom-right of the
//!   viewport at a constant physical size.
//!
//! Owns its fade timer (mirroring HighlightFader): the owner feeds it samples
//! and paints it on top of the notation with the painter in score-logical
//! coordinates.
class TimingFeedbackOverlay
{
public:
  //! \p requestRepaint is invoked whenever the fading gauges change.
  explicit TimingFeedbackOverlay(std::function<void()> requestRepaint);

  TimingFeedbackOverlay(const TimingFeedbackOverlay &) = delete;
  TimingFeedbackOverlay &operator=(const TimingFeedbackOverlay &) = delete;

  //! Record one judged onset: a gauge near \p noteRect (the struck notes'
  //! hugging box, score-logical) and a histogram sample. \p errorMs is the
  //! signed arrival error (− = early, + = late). The gauge goes below the
  //! staff when \p belowStaff (the left hand), else above; \p staffEdgeY is
  //! the staff's facing edge (bottom resp. top line, page-logical y), which
  //! the ruler keeps clear of.
  void addSample(const QRectF &noteRect, double spatium, double errorMs,
                 bool belowStaff, double staffEdgeY);

  //! Forget everything, histogram included — any interruption of play (stop,
  //! a click or swipe, a position jump, a new score) starts the stats over.
  void reset();

  //! Paint gauges and histogram. The painter must be in score-logical
  //! coordinates; \p viewport is the visible logical rect and \p scaling the
  //! zoom (physical px per logical unit), used to keep the histogram HUD a
  //! constant physical size.
  void paint(QPainter &painter, const QRectF &viewport, double scaling) const;

private:
  //! Timer tick: drop fully-faded gauges, stop when none remain, repaint.
  void advance();

  struct Gauge
  {
    double x;       // logical x, centred on the struck notes
    double centerY; // logical y of the ruler's zero mark
    double spatium; // score staff-space unit: sizes the ruler
    double errorMs;
    qint64 startMs;
  };
  void paintGauge(QPainter &painter, const Gauge &gauge,
                  double opacity) const;
  void paintHistogram(QPainter &painter, const QRectF &viewport,
                      double scaling) const;
  void pruneSamples();

  const std::function<void()> _requestRepaint;
  QElapsedTimer _clock; // free-running; timestamps gauges and samples
  QTimer _timer;        // ~60 Hz while gauges are fading

  std::deque<Gauge> _gauges;

  //! The recent error samples backing the histogram; binned (and weighted by
  //! recentness) at paint time, pruned past the window.
  struct Sample
  {
    double errorMs;
    qint64 tMs;
  };
  std::deque<Sample> _samples;
};
} // namespace dgk
