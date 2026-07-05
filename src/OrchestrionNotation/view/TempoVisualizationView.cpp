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
#include "TempoVisualizationView.h"

#include <QFont>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
constexpr double marginPx = 6.0;
constexpr double gutterPx = 52.0; // left strip for the y-axis labels

// Per-hand colour (staff 0, 1, then cycling).
QColor handColor(int staff)
{
  static const QColor palette[] = {QColor("#4F8FE0"), QColor("#E06B5A"),
                                   QColor("#5AB97A"), QColor("#C99A3B")};
  return palette[staff >= 0 ? staff % 4 : 0];
}

// Round up to the next "nice" number (1/2/5 × 10^k) so the autoscaled axis max
// (and its label) is stable and readable rather than jittering every frame.
double niceCeil(double x)
{
  if (x <= 0.0)
    return 1.0;
  const double mag = std::pow(10.0, std::floor(std::log10(x)));
  const double f = x / mag;
  const double nice = f <= 1.0 ? 1.0 : f <= 2.0 ? 2.0 : f <= 5.0 ? 5.0 : 10.0;
  return nice * mag;
}
} // namespace

TempoVisualizationView::TempoVisualizationView(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

void TempoVisualizationView::setModel(TempoVizModel *model)
{
  if (_model == model)
    return;
  if (_model)
    disconnect(_model, nullptr, this, nullptr);
  _model = model;
  if (_model)
    connect(_model, &TempoVizModel::changed, this,
            [this] { update(); });
  emit modelChanged();
  update();
}

void TempoVisualizationView::paint(QPainter *painter)
{
  const double w = width();
  const double h = height();

  painter->setRenderHint(QPainter::Antialiasing, true);
  // Darkened panel + light ink, so labels read against the (dark) backdrop.
  painter->fillRect(QRectF(0, 0, w, h), QColor(0, 0, 0, 110));

  const double bottom = h - marginPx;
  const double top = marginPx;
  const double plotLeft = gutterPx;

  // Shared y-scale: largest tempo across hands in the window, rounded up to a
  // nice value so the axis labels are stable. 0 at the baseline (the user wants
  // min/max labelled even when min is zero).
  double tempoMax = 0.0;
  if (_model)
  {
    for (const auto &[staff, pts] : _model->series())
      for (const auto &p : pts)
        tempoMax = std::max(tempoMax, p.tempo);
    for (const auto &[staff, pts] : _model->smoothed())
      for (const auto &p : pts)
        tempoMax = std::max(tempoMax, p.tempo);
  }
  const double axisMax = niceCeil(tempoMax);
  const auto yOf = [&](double tempo)
  { return bottom - (tempo / axisMax) * (bottom - top); };

  // Y grid + labels at 0, half, max (right-aligned in the gutter).
  QFont labelFont = painter->font();
  labelFont.setPixelSize(10);
  painter->setFont(labelFont);
  for (const double frac : {0.0, 0.5, 1.0})
  {
    const double y = bottom - frac * (bottom - top);
    painter->setPen(
        QPen(QColor(255, 255, 255, frac == 0.0 ? 90 : 40), 1.0));
    painter->drawLine(QPointF(plotLeft, y), QPointF(w, y));
    QString label = QString::number(axisMax * frac, 'g', 3);
    if (frac == 1.0)
      label += " bpm";
    painter->setPen(QColor(235, 235, 235, 220));
    painter->drawText(QRectF(0, y - 8, plotLeft - 5, 16),
                      Qt::AlignRight | Qt::AlignVCenter, label);
  }

  if (!_model || _model->empty() || _model->latestMs() <= 0.0)
    return;

  const double window = TempoVizModel::windowMs;
  const double latest = _model->latestMs();
  const double t0 = latest - window;
  const auto xOf = [&](double t)
  { return plotLeft + (t - t0) / window * (w - plotLeft); };

  painter->setClipRect(QRectF(plotLeft, 0, w - plotLeft, h));

  // The causal per-frame estimate, faint: the live leading edge the smoothed
  // curve hasn't reached yet (and, where they overlap, how far hindsight bends
  // the estimate). Coasting (overdue) stretches drawn dashed.
  for (const auto &[staff, pts] : _model->series())
  {
    const QColor color = handColor(staff);
    for (std::size_t i = 1; i < pts.size(); ++i)
    {
      const auto &a = pts[i - 1];
      const auto &b = pts[i];
      const bool coasting = a.coasting || b.coasting;
      QColor c = color;
      c.setAlpha(coasting ? 60 : 90);
      QPen pen(c, 1.0);
      if (coasting)
        pen.setStyle(Qt::DashLine);
      painter->setPen(pen);
      painter->drawLine(QPointF(xOf(a.tMs), yOf(a.tempo)),
                        QPointF(xOf(b.tMs), yOf(b.tempo)));
    }
  }

  // The smoothed tempo curve — the spline itself, re-fitted on each onset —
  // solid on top. It ends at the last onset; the faint causal trace carries on
  // from there to "now".
  for (const auto &[staff, pts] : _model->smoothed())
  {
    QColor c = handColor(staff);
    c.setAlpha(235);
    painter->setPen(QPen(c, 2.0));
    for (std::size_t i = 1; i < pts.size(); ++i)
    {
      if (pts[i].tMs < t0)
        continue; // scrolled out of the window
      painter->drawLine(QPointF(xOf(pts[i - 1].tMs), yOf(pts[i - 1].tempo)),
                        QPointF(xOf(pts[i].tMs), yOf(pts[i].tempo)));
    }
  }

  painter->setClipping(false);

  // Onset ticks (the inputs the model reacts to), per hand, along the baseline.
  for (const auto &[staff, times] : _model->onsets())
  {
    QColor c = handColor(staff);
    painter->setPen(QPen(c, 1.5));
    for (const double t : times)
    {
      if (t < t0)
        continue;
      const double x = xOf(t);
      painter->drawLine(QPointF(x, bottom), QPointF(x, bottom - 8.0));
    }
  }
}
} // namespace dgk
