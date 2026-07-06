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
#include <array>
#include <cmath>

namespace dgk
{
namespace
{
// The gauge's fixed error range: the ruler always spans ± this many ms, so
// its reading is comparable across notes and sessions. Beyond it = outlier.
constexpr double rangeMs = 250.0;

// Gauge lifetime: readable for a moment, then fading out.
constexpr qint64 holdMs = 800;
constexpr qint64 gaugeLifeMs = 2000;

// Ruler geometry in staff spaces (spatium), so it scales with the score.
constexpr double halfLenSp = 4.0;    // half the ruler's length (= rangeMs)
constexpr double clearanceSp = 1.5;  // gap between the notes and the ruler
constexpr double centerTickSp = 0.8; // half-width of the zero mark
constexpr double endTickSp = 0.4;    // half-width of the range end caps
constexpr double markerRadiusSp = 0.5;

// Histogram HUD, in physical pixels (constant apparent size).
constexpr double hudWidthPx = 170.0;
constexpr double hudHeightPx = 132.0;
constexpr double hudMarginPx = 12.0;
constexpr double hudPaddingPx = 10.0;
constexpr double hudLabelBandPx = 12.0;
constexpr int binCount = 20; // in-range bins; outlier bins sit at both ends

// The histogram's horizon: only the last minute of samples counts, and within
// it recent samples weigh more (exponential decay), so the picture tracks how
// you are playing *now*, not the whole session.
constexpr double sampleWindowMs = 60000.0;
constexpr double recencyTauMs = 20000.0;

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

void TimingFeedbackOverlay::addSample(const QRectF &noteRect, double spatium,
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

  const qint64 now = _clock.elapsed();
  _gauges.push_back({noteRect.center().x(), centerY, spatium, errorMs, now});
  _samples.push_back({errorMs, now});
  pruneSamples();

  if (!_timer.isActive())
    _timer.start();
}

void TimingFeedbackOverlay::reset()
{
  _gauges.clear();
  _samples.clear();
  _timer.stop();
}

void TimingFeedbackOverlay::pruneSamples()
{
  const qint64 now = _clock.elapsed();
  while (!_samples.empty() &&
         static_cast<double>(now - _samples.front().tMs) > sampleWindowMs)
    _samples.pop_front();
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
    const qint64 age = now - gauge.startMs;
    const double opacity =
        age <= holdMs ? 1.0
                      : 1.0 - static_cast<double>(age - holdMs) /
                                  static_cast<double>(gaugeLifeMs - holdMs);
    if (opacity > 0.0)
      paintGauge(painter, gauge, opacity);
  }

  if (!_samples.empty())
    paintHistogram(painter, viewport, scaling);

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
    const double tipY =
        early ? topY - 1.4 * endTickSp * sp - 0.9 * sp
              : bottomY + 1.4 * endTickSp * sp + 0.9 * sp;
    const double baseY = early ? tipY + 0.9 * sp : tipY - 0.9 * sp;
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(QPolygonF{QPointF(x, tipY),
                                  QPointF(x - 0.55 * sp, baseY),
                                  QPointF(x + 0.55 * sp, baseY)});
  }
}

void TimingFeedbackOverlay::paintHistogram(QPainter &painter,
                                           const QRectF &viewport,
                                           double scaling) const
{
  // Dock bottom-right at constant physical size: translate to the panel's
  // logical origin, then undo the zoom so everything below is in physical px.
  painter.save();
  painter.translate(viewport.right() - (hudWidthPx + hudMarginPx) / scaling,
                    viewport.bottom() - (hudHeightPx + hudMarginPx) / scaling);
  painter.scale(1.0 / scaling, 1.0 / scaling);

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(0, 0, 0, 130));
  painter.drawRoundedRect(QRectF(0, 0, hudWidthPx, hudHeightPx), 8, 8);

  // Bin the recent samples, weighting each by recentness: an old note counts
  // for little, the last few notes dominate. Bin [0] = early outliers,
  // [1..binCount] in-range (early → late), [binCount+1] = late outliers.
  const qint64 now = _clock.elapsed();
  std::array<double, binCount + 2> bins{};
  int inWindow = 0;
  for (const Sample &sample : _samples)
  {
    const double age = static_cast<double>(now - sample.tMs);
    if (age > sampleWindowMs)
      continue;
    int bin;
    if (sample.errorMs < -rangeMs)
      bin = 0;
    else if (sample.errorMs >= rangeMs)
      bin = binCount + 1;
    else
      bin = 1 + static_cast<int>((sample.errorMs + rangeMs) /
                                 (2.0 * rangeMs) * binCount);
    bins[static_cast<std::size_t>(std::clamp(bin, 0, binCount + 1))] +=
        std::exp(-age / recencyTauMs);
    ++inWindow;
  }
  const double maxWeight = *std::max_element(bins.begin(), bins.end());
  if (maxWeight <= 0.0)
  {
    painter.restore();
    return;
  }

  // Same vertical convention as the gauges: early at the top, late at the
  // bottom, bars growing rightward, each bin in its error's colour.
  const double plotTop = hudPaddingPx + hudLabelBandPx;
  const double plotBottom = hudHeightPx - hudPaddingPx - hudLabelBandPx;
  const double plotLeft = hudPaddingPx;
  const double plotRight = hudWidthPx - hudPaddingPx;
  const double slotH =
      (plotBottom - plotTop) / static_cast<double>(bins.size());

  for (std::size_t i = 0; i < bins.size(); ++i)
  {
    if (bins[i] <= 0.0)
      continue;
    const bool outlier = i == 0 || i + 1 == bins.size();
    // The bin's centre error, mapped back from its index.
    const double binErrorMs =
        outlier ? rangeMs
                : (static_cast<double>(i) - 0.5) * 2.0 * rangeMs / binCount -
                      rangeMs;
    const double w = (plotRight - plotLeft) * bins[i] / maxWeight;
    const double y = plotTop + static_cast<double>(i) * slotH;
    painter.setBrush(withAlpha(errorColor(binErrorMs), 0.85));
    painter.drawRect(QRectF(plotLeft, y + 0.5, w, slotH - 1.0));
  }

  // The zero-error line, splitting the in-range zone in half, and the range
  // limits (always drawn, so the ± bounds stay visible even when the nearby
  // bins are empty — only the outlier slots live beyond them).
  const double zeroY = plotTop + slotH * (1.0 + binCount / 2.0);
  painter.setPen(QPen(QColor(235, 235, 235, 200), 1.0));
  painter.drawLine(QPointF(plotLeft, zeroY), QPointF(plotRight, zeroY));
  painter.setPen(QPen(QColor(235, 235, 235, 110), 1.0, Qt::DashLine));
  for (const double y : {plotTop + slotH, plotTop + slotH * (1.0 + binCount)})
    painter.drawLine(QPointF(plotLeft, y), QPointF(plotRight, y));

  QFont font = painter.font();
  font.setPixelSize(9);
  painter.setFont(font);
  painter.setPen(QColor(235, 235, 235, 220));
  painter.drawText(QRectF(plotLeft, 1, plotRight - plotLeft, hudLabelBandPx +
                                                                 hudPaddingPx),
                   Qt::AlignLeft | Qt::AlignVCenter, "early");
  painter.drawText(QRectF(plotLeft, 1, plotRight - plotLeft, hudLabelBandPx +
                                                                 hudPaddingPx),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString("n=%1").arg(inWindow));
  painter.drawText(QRectF(plotLeft, plotBottom, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx - 1),
                   Qt::AlignLeft | Qt::AlignVCenter, "late");
  painter.drawText(QRectF(plotLeft, plotBottom, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx - 1),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::fromUtf8("±%1 ms").arg(rangeMs));
  painter.restore();
}
} // namespace dgk
