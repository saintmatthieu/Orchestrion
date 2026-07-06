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
#include "TimingFeedbackOverlay.h"

#include <QFont>
#include <QPainter>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace dgk
{
namespace
{
// The gauge's fixed error range: the ruler always spans ± this many ms, so
// its reading is comparable across notes and sessions. Beyond it = outlier.
constexpr double rangeMs = 250.0;

// Gauge lifetime: readable for a moment, then fading out.
constexpr qint64 holdMs = 10000;
constexpr qint64 gaugeLifeMs = 12000;

// Ruler geometry in staff spaces (spatium), so it scales with the score.
constexpr double halfLenSp = 4.0;    // half the ruler's length (= rangeMs)
constexpr double clearanceSp = 1.5;  // gap between the notes and the ruler
constexpr double centerTickSp = 0.8; // half-width of the zero mark
constexpr double endTickSp = 0.4;    // half-width of the range end caps
constexpr double markerRadiusSp = 0.5;
// A replayed spot (a repeat pass, a retried note) stacks its gauge this far
// beyond the earlier one instead of painting over it.
constexpr double stackStepSp = 2.0 * halfLenSp + 1.5;

// Box-plot HUD, in physical pixels (constant apparent size).
constexpr double hudWidthPx = 2 * 110.0;
constexpr double hudHeightPx = 2 * 132.0;
constexpr double hudMarginPx = 12.0;
constexpr double hudPaddingPx = 10.0;
constexpr double hudLabelBandPx = 12.0;
// A small band past each ± range limit where beyond-scale values get pinned.
constexpr double outlierBandPx = 2 * 8.0;
constexpr double boxHalfWidthPx = 2 * 18.0;
constexpr double whiskerCapHalfWidthPx = 2 * 9.0;

// The stats' horizon: only the last minute of samples counts, and within it
// recent samples weigh more (exponential decay), so the picture tracks how
// you are playing *now*, not the whole session.
constexpr double sampleWindowMs = 60000.0;
constexpr double recencyTauMs = 20000.0;

// The score: 100·e^(−m/scoreRefMs), where m is the scoreQuantile-th
// percentile of |error|. Robust like the box plot but centred on zero, so
// both bias and spread cost points; exact playback (m of a few ms) scores in
// the high 90s, m = scoreRefMs scores ≈ 37.
constexpr double scoreQuantile = 0.8;
constexpr double scoreRefMs = 60.0;
constexpr double scoreBandPx = 36.0; // panel band above the plot

//! The \p q-quantile of |error| over (|error|, weight) pairs; the first value
//! where the running weight reaches the requested share of the total.
std::optional<double>
weightedAbsQuantile(std::vector<std::pair<double, double>> data, double q)
{
  if (data.empty())
    return std::nullopt;
  std::sort(data.begin(), data.end());
  double totalWeight = 0.0;
  for (const auto &[error, weight] : data)
    totalWeight += weight;
  const double target = q * totalWeight;
  double cum = 0.0;
  for (const auto &[error, weight] : data)
  {
    cum += weight;
    if (cum >= target)
      return error;
  }
  return data.back().first;
}

int scoreOf(double absErrorQuantileMs)
{
  return static_cast<int>(
      std::lround(100.0 * std::exp(-absErrorQuantileMs / scoreRefMs)));
}

// Accuracy colour: green when dead on time, grading through yellow/orange to
// red at (and beyond) the gauge's range — the traffic-light hue sweep.
QColor errorColor(double errorMs)
{
  const double t = std::min(std::abs(errorMs) / rangeMs, 1.0);
  return QColor::fromHsvF((1.0 - t) / 3.0, 0.72, 0.82);
}

QColor withAlpha(QColor color, double alpha)
{
  color.setAlphaF(std::clamp(alpha, 0.0, 1.0));
  return color;
}
} // namespace

TimingFeedbackOverlay::TimingFeedbackOverlay(
    std::function<void()> requestRepaint)
    : _requestRepaint(std::move(requestRepaint))
{
  _clock.start();
  _timer.setInterval(16); // ~60 fps while gauges fade
  _timer.callOnTimeout([this] { advance(); });
}

void TimingFeedbackOverlay::addGauge(int staff, double onsetTMs,
                                     const QRectF &noteRect, double spatium,
                                     double errorMs, bool belowStaff,
                                     double staffEdgeY)
{
  // Place the ruler clear of both the struck notes and the staff lines: for
  // the right hand its lower end is min(above the notes, above the staff);
  // mirrored for the left hand below.
  const double clearance = clearanceSp * spatium;
  double centerY;
  if (belowStaff)
  {
    const double top =
        std::max(noteRect.bottom() + clearance, staffEdgeY + clearance);
    centerY = top + halfLenSp * spatium;
  }
  else
  {
    const double bottom =
        std::min(noteRect.top() - clearance, staffEdgeY - clearance);
    centerY = bottom - halfLenSp * spatium;
  }

  // A gauge already at this spot (an earlier repeat pass, a retried note):
  // stack outward — above the first ones for the right hand, below for the
  // left — so every pass stays readable.
  const double x = noteRect.center().x();
  int level = 0;
  for (const Gauge &gauge : _gauges)
    if (gauge.staff == staff && std::abs(gauge.x - x) < 0.5 * spatium)
      ++level;
  const double stackOffset = level * stackStepSp * spatium;
  centerY += belowStaff ? stackOffset : -stackOffset;

  _gauges.push_back({staff, onsetTMs, x, centerY, spatium, errorMs,
                     _clock.elapsed()});
  if (!_persistent && !_timer.isActive())
    _timer.start();
}

void TimingFeedbackOverlay::setPersistent(bool persistent)
{
  if (_persistent == persistent)
    return;
  _persistent = persistent;
  if (_persistent)
    _timer.stop(); // nothing fades; repaints ride the judgment updates
  else if (!_gauges.empty())
  {
    // Fade the accumulated marks out from now rather than dropping them.
    const qint64 now = _clock.elapsed();
    for (Gauge &gauge : _gauges)
      gauge.startMs = now;
    _timer.start();
  }
  _requestRepaint();
}

QString TimingFeedbackOverlay::gaugeInfoAt(const QPointF &logicalPos) const
{
  // Newest first, so coinciding marks report the latest pass.
  for (auto it = _gauges.rbegin(); it != _gauges.rend(); ++it)
  {
    const Gauge &gauge = *it;
    const double sp = gauge.spatium;
    const QRectF hitRect(gauge.x - 1.5 * sp,
                         gauge.centerY - (halfLenSp + 2.0) * sp, 3.0 * sp,
                         2.0 * (halfLenSp + 2.0) * sp);
    if (!hitRect.contains(logicalPos))
      continue;
    const int ms = static_cast<int>(std::lround(std::abs(gauge.errorMs)));
    if (ms == 0)
      return QStringLiteral("on time");
    return QStringLiteral("%1 ms %2")
        .arg(ms)
        .arg(gauge.errorMs < 0.0 ? QStringLiteral("early")
                                 : QStringLiteral("late"));
  }
  return {};
}

void TimingFeedbackOverlay::updateJudgments(
    int staff, const std::vector<TempoFollower::Judgment> &window)
{
  const qint64 now = _clock.elapsed();
  auto &samples = _samples[staff];
  auto &takeSamples = _takeSamples[staff];
  for (const TempoFollower::Judgment &judgment : window)
  {
    if (const auto it = samples.find(judgment.tMs); it != samples.end())
      it->second.errorMs = judgment.errorMs;
    else
      samples.emplace(judgment.tMs, Sample{judgment.errorMs, now});
    takeSamples[judgment.tMs] = judgment.errorMs;
  }

  for (Gauge &gauge : _gauges)
  {
    if (gauge.staff != staff)
      continue;
    const auto it = std::find_if(window.begin(), window.end(),
                                 [&gauge](const TempoFollower::Judgment &j)
                                 { return j.tMs == gauge.onsetTMs; });
    if (it != window.end())
      gauge.errorMs = it->errorMs;
  }

  pruneSamples();
  _requestRepaint();
}

void TimingFeedbackOverlay::reset()
{
  _gauges.clear();
  _samples.clear();
  _takeSamples.clear();
  _timer.stop();
}

std::optional<double> TimingFeedbackOverlay::recentAbsErrorQuantile() const
{
  const qint64 now = _clock.elapsed();
  std::vector<std::pair<double, double>> data;
  for (const auto &[staff, samples] : _samples)
    for (const auto &[onsetTMs, sample] : samples)
    {
      const double age = static_cast<double>(now - sample.arrivalMs);
      if (age <= sampleWindowMs)
        data.emplace_back(std::abs(sample.errorMs),
                          std::exp(-age / recencyTauMs));
    }
  return weightedAbsQuantile(std::move(data), scoreQuantile);
}

std::optional<int> TimingFeedbackOverlay::takeScore() const
{
  std::vector<std::pair<double, double>> data;
  for (const auto &[staff, samples] : _takeSamples)
    for (const auto &[onsetTMs, errorMs] : samples)
      data.emplace_back(std::abs(errorMs), 1.0);
  const auto quantile = weightedAbsQuantile(std::move(data), scoreQuantile);
  if (!quantile)
    return std::nullopt;
  return scoreOf(*quantile);
}

void TimingFeedbackOverlay::pruneSamples()
{
  const qint64 now = _clock.elapsed();
  for (auto &[staff, samples] : _samples)
    for (auto it = samples.begin(); it != samples.end();)
      it = static_cast<double>(now - it->second.arrivalMs) > sampleWindowMs
               ? samples.erase(it)
               : std::next(it);
}

void TimingFeedbackOverlay::advance()
{
  const qint64 now = _clock.elapsed();
  while (!_gauges.empty() && now - _gauges.front().startMs >= gaugeLifeMs)
    _gauges.pop_front();
  pruneSamples();
  if (_gauges.empty())
    _timer.stop();
  _requestRepaint();
}

void TimingFeedbackOverlay::paint(QPainter &painter, const QRectF &viewport,
                                  double scaling) const
{
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setOpacity(1.0);

  const qint64 now = _clock.elapsed();
  for (const Gauge &gauge : _gauges)
  {
    if (gauge.x < viewport.left() || gauge.x > viewport.right())
      continue; // scrolled out of view (persistent marks can be many)
    const qint64 age = now - gauge.startMs;
    const double opacity =
        _persistent ? 1.0
        : age <= holdMs
            ? 1.0
            : 1.0 - static_cast<double>(age - holdMs) /
                        static_cast<double>(gaugeLifeMs - holdMs);
    if (opacity > 0.0)
      paintGauge(painter, gauge, opacity);
  }

  paintBoxPlot(painter, viewport, scaling);

  painter.restore();
}

void TimingFeedbackOverlay::paintGauge(QPainter &painter, const Gauge &gauge,
                                       double opacity) const
{
  const double sp = gauge.spatium;
  const double x = gauge.x;
  const double centerY = gauge.centerY;
  const double topY = centerY - halfLenSp * sp;
  const double bottomY = centerY + halfLenSp * sp;

  // The ruler: a thin vertical line with end caps and a stronger zero mark.
  const QColor ink(40, 40, 40);
  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(withAlpha(ink, 0.55 * opacity), 0.15 * sp));
  painter.drawLine(QPointF(x, topY), QPointF(x, bottomY));
  for (const double y : {topY, bottomY})
    painter.drawLine(QPointF(x - endTickSp * sp, y),
                     QPointF(x + endTickSp * sp, y));
  painter.setPen(QPen(withAlpha(ink, 0.8 * opacity), 0.2 * sp));
  painter.drawLine(QPointF(x - centerTickSp * sp, centerY),
                   QPointF(x + centerTickSp * sp, centerY));

  // The landing marker: high = early, low = late (+y is down, and a positive
  // error means late, so the offset maps directly).
  const QColor color = withAlpha(errorColor(gauge.errorMs), 0.95 * opacity);
  if (std::abs(gauge.errorMs) <= rangeMs)
  {
    const double y = centerY + gauge.errorMs / rangeMs * halfLenSp * sp;
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(x, y), markerRadiusSp * sp,
                        markerRadiusSp * sp);
  }
  else
  {
    // Outlier: an arrowhead just past the ruler's end, pointing out.
    const bool early = gauge.errorMs < 0.0;
    const double tipY = early ? topY - 1.4 * endTickSp * sp - 0.9 * sp
                              : bottomY + 1.4 * endTickSp * sp + 0.9 * sp;
    const double baseY = early ? tipY + 0.9 * sp : tipY - 0.9 * sp;
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(QPolygonF{QPointF(x, tipY),
                                  QPointF(x - 0.55 * sp, baseY),
                                  QPointF(x + 0.55 * sp, baseY)});
  }
}

void TimingFeedbackOverlay::paintBoxPlot(QPainter &painter,
                                         const QRectF &viewport,
                                         double scaling) const
{
  // Gather the in-window samples (at their latest revised errors) with their
  // recency weights: an old note counts for little, the last few notes
  // dominate the statistics.
  const qint64 now = _clock.elapsed();
  std::vector<std::pair<double /*errorMs*/, double /*weight*/>> data;
  for (const auto &[staff, samples] : _samples)
    for (const auto &[onsetTMs, sample] : samples)
    {
      const double age = static_cast<double>(now - sample.arrivalMs);
      if (age <= sampleWindowMs)
        data.emplace_back(sample.errorMs, std::exp(-age / recencyTauMs));
    }
  if (data.empty())
    return;
  std::sort(data.begin(), data.end());

  // Weighted quantiles: the first sample where the running weight reaches the
  // requested share of the total.
  double totalWeight = 0.0;
  for (const auto &[error, weight] : data)
    totalWeight += weight;
  const auto quantile = [&data, totalWeight](double q)
  {
    const double target = q * totalWeight;
    double cum = 0.0;
    for (const auto &[error, weight] : data)
    {
      cum += weight;
      if (cum >= target)
        return error;
    }
    return data.back().first;
  };
  const double median = quantile(0.5);
  const double q1 = quantile(0.25);
  const double q3 = quantile(0.75);

  // Tukey whiskers: out to the farthest samples within 1.5 IQR of the box;
  // anything beyond is an outlier dot.
  const double fenceLo = q1 - 1.5 * (q3 - q1);
  const double fenceHi = q3 + 1.5 * (q3 - q1);
  double whiskerLo = median;
  double whiskerHi = median;
  std::vector<double> outliers;
  for (const auto &[error, weight] : data)
  {
    if (error < fenceLo || error > fenceHi)
      outliers.push_back(error);
    else
    {
      whiskerLo = std::min(whiskerLo, error);
      whiskerHi = std::max(whiskerHi, error);
    }
  }

  // Dock bottom-right at constant physical size: translate to the panel's
  // logical origin, then undo the zoom so everything below is in physical px.
  const double panelHeight = scoreBandPx + hudHeightPx;
  painter.save();
  painter.translate(viewport.right() - (hudWidthPx + hudMarginPx) / scaling,
                    viewport.bottom() - (panelHeight + hudMarginPx) / scaling);
  painter.scale(1.0 / scaling, 1.0 / scaling);

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(0, 0, 0, 130));
  painter.drawRoundedRect(QRectF(0, 0, hudWidthPx, panelHeight), 8, 8);

  // Same vertical convention as the gauges: early at the top, late at the
  // bottom. Beyond-scale values get pinned into a small band past the ±
  // range limits. The score band sits above the plot.
  const double plotTop = scoreBandPx + hudPaddingPx + hudLabelBandPx;
  const double plotBottom = panelHeight - hudPaddingPx - hudLabelBandPx;
  const double plotLeft = hudPaddingPx;
  const double plotRight = hudWidthPx - hudPaddingPx;
  const double limitTop = plotTop + outlierBandPx;
  const double limitBottom = plotBottom - outlierBandPx;
  const auto yOf = [&](double errorMs)
  {
    if (errorMs < -rangeMs)
      return plotTop + 0.5 * outlierBandPx;
    if (errorMs > rangeMs)
      return plotBottom - 0.5 * outlierBandPx;
    return limitTop +
           (errorMs + rangeMs) / (2.0 * rangeMs) * (limitBottom - limitTop);
  };

  // The zero-error line and the always-visible ± range limits.
  painter.setPen(QPen(QColor(235, 235, 235, 200), 1.0));
  painter.drawLine(QPointF(plotLeft, yOf(0.0)), QPointF(plotRight, yOf(0.0)));
  painter.setPen(QPen(QColor(235, 235, 235, 110), 1.0, Qt::DashLine));
  for (const double y : {limitTop, limitBottom})
    painter.drawLine(QPointF(plotLeft, y), QPointF(plotRight, y));

  // Whiskers: the stem and its caps, in neutral ink.
  const double cx = hudWidthPx / 2.0;
  painter.setPen(QPen(QColor(235, 235, 235, 190), 1.0));
  painter.drawLine(QPointF(cx, yOf(whiskerLo)), QPointF(cx, yOf(whiskerHi)));
  for (const double w : {whiskerLo, whiskerHi})
    painter.drawLine(QPointF(cx - whiskerCapHalfWidthPx, yOf(w)),
                     QPointF(cx + whiskerCapHalfWidthPx, yOf(w)));

  // The quartile box, in the median's accuracy colour, and the median line.
  const QColor color = errorColor(median);
  painter.setPen(QPen(withAlpha(color, 0.9), 1.0));
  painter.setBrush(withAlpha(color, 0.35));
  painter.drawRect(QRectF(cx - boxHalfWidthPx, yOf(q1), 2.0 * boxHalfWidthPx,
                          yOf(q3) - yOf(q1)));
  painter.setPen(QPen(withAlpha(color, 1.0), 2.0));
  painter.drawLine(QPointF(cx - boxHalfWidthPx, yOf(median)),
                   QPointF(cx + boxHalfWidthPx, yOf(median)));

  // Outlier dots, slightly jittered sideways so a cluster reads as one.
  painter.setPen(Qt::NoPen);
  for (std::size_t i = 0; i < outliers.size(); ++i)
  {
    painter.setBrush(withAlpha(errorColor(outliers[i]), 0.85));
    const double x = cx + (static_cast<double>(i % 5) - 2.0) * 4.0;
    painter.drawEllipse(QPointF(x, yOf(outliers[i])), 2.0, 2.0);
  }

  QFont font = painter.font();
  font.setPixelSize(9);
  painter.setFont(font);
  painter.setPen(QColor(235, 235, 235, 220));
  painter.drawText(QRectF(plotLeft, scoreBandPx + 1, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx),
                   Qt::AlignLeft | Qt::AlignVCenter, "early");
  painter.drawText(QRectF(plotLeft, scoreBandPx + 1, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString("n=%1").arg(static_cast<int>(data.size())));
  painter.drawText(QRectF(plotLeft, plotBottom, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx - 1),
                   Qt::AlignLeft | Qt::AlignVCenter, "late");
  painter.drawText(QRectF(plotLeft, plotBottom, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx - 1),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::fromUtf8("±%1 ms").arg(rangeMs));

  // The live score, big, in its accuracy colour (the same 0–100 the final
  // popup reports, but recency-weighted).
  if (const auto quantile = recentAbsErrorQuantile())
  {
    painter.drawText(QRectF(plotLeft, 0, plotRight - plotLeft, scoreBandPx),
                     Qt::AlignLeft | Qt::AlignVCenter, "score");
    QFont scoreFont = painter.font();
    scoreFont.setPixelSize(26);
    scoreFont.setBold(true);
    painter.setFont(scoreFont);
    painter.setPen(withAlpha(errorColor(*quantile), 1.0));
    painter.drawText(QRectF(plotLeft, 0, plotRight - plotLeft, scoreBandPx),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QString::number(scoreOf(*quantile)));
  }
  painter.restore();
}
} // namespace dgk
