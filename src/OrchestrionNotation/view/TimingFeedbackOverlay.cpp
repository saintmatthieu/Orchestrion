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

#include <draw/painter.h>
#include <engraving/dom/engravingitem.h>
#include <engraving/dom/spanner.h>
#include <engraving/rendering/iscorerenderer.h>

#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace dgk
{
namespace
{
// The gauge's fixed error range: the ruler always spans ± this many ms, so
// its reading is comparable across notes and sessions. Beyond it = outlier.
constexpr double rangeMs = 250.0;
// The dynamics scale sharing the same ruler (and the dynamics box plot's
// axis): ± this velocity fraction, + (up) = too loud.
constexpr double dynamicsRange = 0.25;

// Gauge lifetime: readable for a moment, then fading out.
constexpr qint64 holdMs = 10000;
constexpr qint64 gaugeLifeMs = 12000;
// The per-onset ruler gauges overlap the deviation ribbon, which now carries
// the same information — hidden for now, one flag away from returning.
// (Their data still feeds the shadows, the ribbon and the tooltips.)
constexpr bool showGaugeRulers = false;

//! The current engraved centre of a set of items (they move when the layout
//! warps), or \p fallback when there are none.
double unitedItemsCenterX(
    const std::vector<mu::engraving::EngravingItem *> &items, double fallback)
{
  if (items.empty())
    return fallback;
  double left = std::numeric_limits<double>::max();
  double right = std::numeric_limits<double>::lowest();
  for (const mu::engraving::EngravingItem *item : items)
  {
    const muse::RectF rect = item->pageBoundingRect();
    left = std::min(left, rect.left());
    right = std::max(right, rect.right());
  }
  return 0.5 * (left + right);
}
} // namespace

double TimingFeedbackOverlay::gaugeAnchorX(const Gauge &gauge) const
{
  return unitedItemsCenterX(gauge.items, gauge.x);
}

double TimingFeedbackOverlay::ribbonAnchorX(const RibbonPoint &point) const
{
  return unitedItemsCenterX(point.items, point.x);
}

namespace
{
// Ruler geometry in staff spaces (spatium), so it scales with the score.
constexpr double halfLenSp = 4.0;      // half the ruler's length (= rangeMs)
constexpr double clearanceSp = 1.5;    // gap between the notes and the ruler
constexpr double centerTickSp = 0.8;   // half-width of the zero mark
constexpr double endTickSp = 0.4;      // half-width of the range end caps
constexpr double markerRadiusSp = 0.5; // the timing dot
constexpr double ringRadiusSp = 0.85;  // the dynamics ring around it
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

// Ticks → logical px in the fork's time-proportional layout: the
// HORIZONTAL_FIXED mode gives widthOfSegmentCell (3) staff spaces per global
// quantum (a sixteenth = 120 ticks). Keep in sync with
// MasterScore::widthOfSegmentCell and the fork's calculateQuantumCell.
constexpr double shadowCellWidthSp = 3.0;
constexpr double shadowQuantumTicks = 120.0;
// The performance copy's translucency.
constexpr double shadowActualAlpha = 0.8;

// A component's score: 100·e^(−m/ref), where m is the scoreQuantile-th
// percentile of its |error| — robust like the box plot but centred on zero,
// so both bias and spread cost points — and ref that component's reference
// error (scoring ≈ 37). Timing-like components (tempo, sync) measure ms;
// the dynamics component measures velocity fractions (0..1).
constexpr double scoreQuantile = 0.8;
constexpr double scoreRefMs = 60.0;
constexpr double dynamicsScoreRef = 0.12;
// Panel band above the plot: the combined score plus a component breakdown.
constexpr double scoreBandPx = 52.0;
constexpr double scoreMainRowPx = 34.0;

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

int scoreOf(double absErrorQuantile, double ref)
{
  return static_cast<int>(
      std::lround(100.0 * std::exp(-absErrorQuantile / ref)));
}

//! One component of the score: its display label, error quantile (absent =
//! no samples, component not applicable) and reference error.
struct Component
{
  const char *label;
  std::optional<double> quantile;
  double ref;
};

//! The component scores as a display line, e.g. "tempo 87 · sync 92" — used
//! both in the HUD's score band (recent window) and next to the final banner
//! score (whole take). Empty with fewer than two components: a lone
//! sub-score would just repeat the combined one.
QString breakdownText(const std::vector<Component> &components)
{
  const auto present =
      std::count_if(components.begin(), components.end(),
                    [](const Component &c) { return c.quantile.has_value(); });
  if (present < 2)
    return {};
  QString text;
  for (const Component &component : components)
  {
    if (!component.quantile)
      continue;
    if (!text.isEmpty())
      text += QString::fromUtf8(" · ");
    text += QStringLiteral("%1 %2")
                .arg(component.label)
                .arg(scoreOf(*component.quantile, component.ref));
  }
  return text;
}

//! The combined score: the plain average of the available component scores
//! (a single-staff piece has no sync component; a velocity-less controller
//! yields no dynamics samples). A geometric mean — i.e. averaging the
//! normalized errors before the exp map — would punish one bad component
//! harder; a knob for later. Also yields the mean error in units of the
//! components' references, for colouring the display.
struct Composite
{
  int score;
  double meanErrorRatio;
};
std::optional<Composite> combineScores(const std::vector<Component> &components)
{
  double scoreSum = 0.0;
  double ratioSum = 0.0;
  int count = 0;
  for (const Component &component : components)
    if (component.quantile)
    {
      scoreSum += scoreOf(*component.quantile, component.ref);
      ratioSum += *component.quantile / component.ref;
      ++count;
    }
  if (count == 0)
    return std::nullopt;
  return Composite{static_cast<int>(std::lround(scoreSum / count)),
                   ratioSum / count};
}

// Accuracy colour: green when dead on, grading through yellow/orange to red
// at (and beyond) the scale's range — the traffic-light hue sweep. \p t is
// |error| as a fraction of the applicable range.
QColor ratioColor(double t)
{
  return QColor::fromHsvF((1.0 - std::clamp(t, 0.0, 1.0)) / 3.0, 0.72, 0.82);
}

QColor errorColor(double errorMs)
{
  return ratioColor(std::abs(errorMs) / rangeMs);
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

void TimingFeedbackOverlay::setShadowsEnabled(bool enabled)
{
  _shadowsEnabled = enabled;
  _requestRepaint();
}

void TimingFeedbackOverlay::setWarpProgress(double progress)
{
  _warpProgress = std::clamp(progress, 0.0, 1.0);
  _requestRepaint();
}

std::optional<double> TimingFeedbackOverlay::takeErrorAt(int staff,
                                                         double tMs) const
{
  const auto staffIt = _takeSamples.find(staff);
  if (staffIt == _takeSamples.end())
    return std::nullopt;
  const auto it = staffIt->second.find(tMs);
  if (it == staffIt->second.end())
    return std::nullopt;
  return it->second;
}

void TimingFeedbackOverlay::addGauge(
    int staff, double onsetTMs, const QRectF &noteRect, double spatium,
    bool belowStaff, double staffEdgeY,
    std::vector<mu::engraving::EngravingItem *> items)
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

  // The deviation ribbon: one take-wide point per judged onset (values are
  // revised by updateJudgments), on the lane's *stable* zero line — the
  // nominal ruler centre, unlike centerY which avoids the struck notes.
  _ribbonBaselineY[staff] =
      belowStaff ? staffEdgeY + (clearanceSp + halfLenSp) * spatium
                 : staffEdgeY - (clearanceSp + halfLenSp) * spatium;
  _ribbon[staff].push_back({onsetTMs, items, x, spatium});

  _gauges.push_back({staff, onsetTMs, x, centerY, spatium, 0.0, false,
                     std::nullopt, std::move(items), 0.0, 0.0, 0.0,
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
    if (!gauge.judged)
      continue;
    const double sp = gauge.spatium;
    const QRectF hitRect(gaugeAnchorX(gauge) - 1.5 * sp,
                         gauge.centerY - (halfLenSp + 2.0) * sp, 3.0 * sp,
                         2.0 * (halfLenSp + 2.0) * sp);
    if (!hitRect.contains(logicalPos))
      continue;
    const int ms = static_cast<int>(std::lround(std::abs(gauge.errorMs)));
    QString info = ms == 0
                       ? QStringLiteral("on time")
                       : QStringLiteral("%1 ms %2")
                             .arg(ms)
                             .arg(gauge.errorMs < 0.0 ? QStringLiteral("early")
                                                      : QStringLiteral("late"));
    if (gauge.dynamicsError)
    {
      const int pct =
          static_cast<int>(std::lround(std::abs(*gauge.dynamicsError) * 100));
      info += QString::fromUtf8(" · ") +
              (pct == 0 ? QStringLiteral("even loudness")
                        : QStringLiteral("%1 % too %2")
                              .arg(pct)
                              .arg(*gauge.dynamicsError > 0.0
                                       ? QStringLiteral("loud")
                                       : QStringLiteral("soft")));
    }
    if (gauge.bpm > 0.0)
      info += QString::fromUtf8(" · %1 bpm")
                  .arg(static_cast<int>(std::lround(gauge.bpm)));
    return info;
  }
  return {};
}

void TimingFeedbackOverlay::upsertSamples(
    int staff, const std::vector<TempoFollower::Judgment> &window,
    SampleMap &samples, TakeSampleMap &takeSamples)
{
  const qint64 now = _clock.elapsed();
  auto &staffSamples = samples[staff];
  auto &staffTakeSamples = takeSamples[staff];
  for (const TempoFollower::Judgment &judgment : window)
  {
    if (const auto it = staffSamples.find(judgment.tMs);
        it != staffSamples.end())
      it->second.errorMs = judgment.errorMs;
    else
      staffSamples.emplace(judgment.tMs, Sample{judgment.errorMs, now});
    staffTakeSamples[judgment.tMs] = judgment.errorMs;
  }
}

void TimingFeedbackOverlay::updateJudgments(
    int staff, const std::vector<TempoFollower::Judgment> &window)
{
  upsertSamples(staff, window, _samples, _takeSamples);

  for (Gauge &gauge : _gauges)
  {
    if (gauge.staff != staff)
      continue;
    const auto it = std::find_if(window.begin(), window.end(),
                                 [&gauge](const TempoFollower::Judgment &j)
                                 { return j.tMs == gauge.onsetTMs; });
    if (it != window.end())
    {
      gauge.errorMs = it->errorMs;
      gauge.warpTicks = it->warpTicks;
      gauge.errorTicks = it->errorTicks;
      gauge.bpm = it->bpm;
      gauge.judged = true;
    }
  }

  if (const auto ribbonIt = _ribbon.find(staff); ribbonIt != _ribbon.end())
    for (RibbonPoint &point : ribbonIt->second)
    {
      const auto it = std::find_if(window.begin(), window.end(),
                                   [&point](const TempoFollower::Judgment &j)
                                   { return j.tMs == point.tMs; });
      if (it != window.end())
      {
        point.warpMs = it->warpMs;
        point.errorMs = it->errorMs;
        point.bpm = it->bpm;
        point.judged = true;
      }
    }

  pruneSamples();
  _requestRepaint();
}

void TimingFeedbackOverlay::updateDynamicsJudgments(
    int staff, const std::vector<TempoFollower::Judgment> &window)
{
  upsertSamples(staff, window, _dynamicsSamples, _takeDynamicsSamples);

  for (Gauge &gauge : _gauges)
  {
    if (gauge.staff != staff)
      continue;
    const auto it = std::find_if(window.begin(), window.end(),
                                 [&gauge](const TempoFollower::Judgment &j)
                                 { return j.tMs == gauge.onsetTMs; });
    if (it != window.end())
      gauge.dynamicsError = it->errorMs;
  }

  pruneSamples();
  _requestRepaint();
}

void TimingFeedbackOverlay::addSyncSample(double tMs, double errorMs)
{
  Q_UNUSED(tMs);
  _syncSamples.push_back({errorMs, _clock.elapsed()});
  _takeSyncErrors.push_back(errorMs);
  pruneSamples();
  _requestRepaint();
}

void TimingFeedbackOverlay::clearSyncStats()
{
  _syncSamples.clear();
  _takeSyncErrors.clear();
  _requestRepaint();
}

void TimingFeedbackOverlay::clearDynamicsStats()
{
  _dynamicsSamples.clear();
  _takeDynamicsSamples.clear();
  _requestRepaint();
}

void TimingFeedbackOverlay::reset()
{
  _gauges.clear();
  _samples.clear();
  _takeSamples.clear();
  _syncSamples.clear();
  _takeSyncErrors.clear();
  _dynamicsSamples.clear();
  _takeDynamicsSamples.clear();
  _ribbon.clear();
  _ribbonBaselineY.clear();
  _timer.stop();
}

std::optional<double>
TimingFeedbackOverlay::recentQuantile(const SampleMap &samples) const
{
  const qint64 now = _clock.elapsed();
  std::vector<std::pair<double, double>> data;
  for (const auto &[staff, staffSamples] : samples)
    for (const auto &[onsetTMs, sample] : staffSamples)
    {
      const double age = static_cast<double>(now - sample.arrivalMs);
      if (age <= sampleWindowMs)
        data.emplace_back(std::abs(sample.errorMs),
                          std::exp(-age / recencyTauMs));
    }
  return weightedAbsQuantile(std::move(data), scoreQuantile);
}

std::optional<double>
TimingFeedbackOverlay::takeQuantile(const TakeSampleMap &samples)
{
  std::vector<std::pair<double, double>> data;
  for (const auto &[staff, staffSamples] : samples)
    for (const auto &[onsetTMs, error] : staffSamples)
      data.emplace_back(std::abs(error), 1.0);
  return weightedAbsQuantile(std::move(data), scoreQuantile);
}

std::optional<double> TimingFeedbackOverlay::recentSyncAbsErrorQuantile() const
{
  const qint64 now = _clock.elapsed();
  std::vector<std::pair<double, double>> data;
  for (const Sample &sample : _syncSamples)
  {
    const double age = static_cast<double>(now - sample.arrivalMs);
    if (age <= sampleWindowMs)
      data.emplace_back(std::abs(sample.errorMs),
                        std::exp(-age / recencyTauMs));
  }
  return weightedAbsQuantile(std::move(data), scoreQuantile);
}

std::optional<double> TimingFeedbackOverlay::takeSyncAbsErrorQuantile() const
{
  std::vector<std::pair<double, double>> data;
  for (const double errorMs : _takeSyncErrors)
    data.emplace_back(std::abs(errorMs), 1.0);
  return weightedAbsQuantile(std::move(data), scoreQuantile);
}

std::optional<int> TimingFeedbackOverlay::takeFinalScore() const
{
  const auto composite = combineScores(
      {{"tempo", takeQuantile(_takeSamples), scoreRefMs},
       {"sync", takeSyncAbsErrorQuantile(), scoreRefMs},
       {"dyn", takeQuantile(_takeDynamicsSamples), dynamicsScoreRef}});
  if (!composite)
    return std::nullopt;
  return composite->score;
}

QString TimingFeedbackOverlay::takeScoreBreakdown() const
{
  return breakdownText(
      {{"tempo", takeQuantile(_takeSamples), scoreRefMs},
       {"sync", takeSyncAbsErrorQuantile(), scoreRefMs},
       {"dyn", takeQuantile(_takeDynamicsSamples), dynamicsScoreRef}});
}

void TimingFeedbackOverlay::pruneSamples()
{
  const qint64 now = _clock.elapsed();
  for (SampleMap *map : {&_samples, &_dynamicsSamples})
    for (auto &[staff, samples] : *map)
      for (auto it = samples.begin(); it != samples.end();)
        it = static_cast<double>(now - it->second.arrivalMs) > sampleWindowMs
                 ? samples.erase(it)
                 : std::next(it);
  while (!_syncSamples.empty() &&
         static_cast<double>(now - _syncSamples.front().arrivalMs) >
             sampleWindowMs)
    _syncSamples.pop_front();
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

  paintRibbon(painter, viewport);

  const qint64 now = _clock.elapsed();
  for (const Gauge &gauge : _gauges)
  {
    if (!gauge.judged)
      continue; // pending: its verdict arrives with the spline warm-up
    const double anchorX = gaugeAnchorX(gauge);
    if (anchorX < viewport.left() || anchorX > viewport.right())
      continue; // scrolled out of view (persistent marks can be many)
    const qint64 age = now - gauge.startMs;
    const double opacity =
        _persistent     ? 1.0
        : age <= holdMs ? 1.0
                        : 1.0 - static_cast<double>(age - holdMs) /
                                    static_cast<double>(gaugeLifeMs - holdMs);
    if (opacity <= 0.0)
      continue;
    if (_shadowsEnabled)
      paintShadows(painter, gauge, opacity);
    if (showGaugeRulers)
      paintGauge(painter, gauge, anchorX, opacity);
  }

  paintBoxPlots(painter, viewport, scaling);

  painter.restore();
}

void TimingFeedbackOverlay::paintShadows(QPainter &painter, const Gauge &gauge,
                                         double opacity) const
{
  if (gauge.items.empty())
    return;

  // The performance copy sits at the note's *absolute* performance position:
  // the full tempo warp plus the residual while the score shows ideal
  // spacing, only the residual once the layout has baked the warp in — so
  // during the bake animation the coloured notes stay put and the page
  // morphs to meet them.
  const double pxPerTick =
      shadowCellWidthSp * gauge.spatium / shadowQuantumTicks;
  const double dx = ((1.0 - _warpProgress) * gauge.warpTicks +
                     gauge.errorTicks) *
                    pxPerTick;

  muse::draw::Painter musePainter(&painter, "timing-shadows");
  painter.setOpacity(shadowActualAlpha * opacity);
  const QColor color = errorColor(gauge.errorMs);
  const auto drawOne = [&](mu::engraving::EngravingItem *item)
  {
    const muse::PointF position = item->pagePos() + muse::PointF(dx, 0.0);
    const muse::draw::Color savedColor = item->color();
    item->setColor(muse::draw::Color::fromQColor(color));
    musePainter.translate(position);
    item->renderer()->drawItem(item, &musePainter);
    musePainter.translate(-position);
    item->setColor(savedColor);
  };
  for (mu::engraving::EngravingItem *item : gauge.items)
  {
    // A spanner (a struck note's tie) has no direct drawing — the renderer
    // asserts on it. Its visual is its laid-out segments: draw those.
    if (const auto spanner = dynamic_cast<mu::engraving::Spanner *>(item))
      for (mu::engraving::SpannerSegment *segment : spanner->spannerSegments())
        drawOne(segment);
    else
      drawOne(item);
  }
  painter.setOpacity(1.0);
}

double TimingFeedbackOverlay::ribbonMaxAbsMs() const
{
  // The take's largest actual deviation fills the band (± halfLenSp staff
  // spaces), never zoomed in past the gauges' range.
  double maxAbsMs = rangeMs;
  for (const auto &[staff, points] : _ribbon)
    for (const RibbonPoint &point : points)
      if (point.judged)
        maxAbsMs = std::max(maxAbsMs, std::abs(point.warpMs + point.errorMs));
  return maxAbsMs;
}

QString TimingFeedbackOverlay::ribbonInfoAt(const QPointF &logicalPos) const
{
  const double maxAbsMs = ribbonMaxAbsMs();
  for (const auto &[staff, allPoints] : _ribbon)
  {
    std::vector<const RibbonPoint *> points;
    points.reserve(allPoints.size());
    for (const RibbonPoint &point : allPoints)
      if (point.judged)
        points.push_back(&point);
    if (points.size() < 2)
      continue;
    const auto baselineIt = _ribbonBaselineY.find(staff);
    if (baselineIt == _ribbonBaselineY.end())
      continue;
    const double baseY = baselineIt->second;
    const double sp = points.back()->spatium;
    const double pxPerMs = halfLenSp * sp / maxAbsMs;

    double prevX = ribbonAnchorX(*points.front());
    for (std::size_t i = 1; i < points.size(); ++i)
    {
      const double x0 = prevX;
      const double x1 = ribbonAnchorX(*points[i]);
      prevX = x1;
      if (x1 <= x0) // a repeat pass rewinds x: not a curve segment
        continue;
      if (logicalPos.x() < x0 || logicalPos.x() > x1)
        continue;
      const RibbonPoint &a = *points[i - 1];
      const RibbonPoint &b = *points[i];
      const double t = (logicalPos.x() - x0) / (x1 - x0);
      const double warpMs = a.warpMs + t * (b.warpMs - a.warpMs);
      if (std::abs(logicalPos.y() - (baseY + warpMs * pxPerMs)) > 1.2 * sp)
        continue;
      // The tempo along the curve — it evolves with the gradient, while the
      // height reads as the accumulated deviation.
      const double bpm = a.bpm + t * (b.bpm - a.bpm);
      QString info = QString::fromUtf8("%1 bpm").arg(
          static_cast<int>(std::lround(bpm)));
      const int deviation =
          static_cast<int>(std::lround(std::abs(warpMs)));
      if (deviation > 0)
        info += QString::fromUtf8(" · %1 ms %2")
                    .arg(deviation)
                    .arg(warpMs > 0.0 ? QStringLiteral("behind")
                                      : QStringLiteral("ahead"));
      return info;
    }
  }
  return {};
}

void TimingFeedbackOverlay::paintRibbon(QPainter &painter,
                                        const QRectF &viewport) const
{
  const double maxAbsMs = ribbonMaxAbsMs();

  const QColor ink(40, 40, 40);
  for (const auto &[staff, points] : _ribbon)
  {
    if (points.empty())
      continue;
    const auto baselineIt = _ribbonBaselineY.find(staff);
    if (baselineIt == _ribbonBaselineY.end())
      continue;
    const double baseY = baselineIt->second;
    const double sp = points.back().spatium;
    const double pxPerMs = halfLenSp * sp / maxAbsMs;

    // Live anchors, split into strokes where x runs backward (repeat passes
    // draw as separate overlapping strokes, like the stacked gauges).
    struct Node
    {
      double x;
      double curveY; // the fitted deviation — the interpolation
      double dotY;   // + residual — where the onset actually landed
      double errorMs;
    };
    std::vector<std::vector<Node>> strokes(1);
    double prevX = std::numeric_limits<double>::lowest();
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    for (const RibbonPoint &point : points)
    {
      if (!point.judged)
        continue; // pending: its verdict arrives with the spline warm-up
      const double x = ribbonAnchorX(point);
      if (x < prevX && !strokes.back().empty())
        strokes.emplace_back();
      prevX = x;
      minX = std::min(minX, x);
      maxX = std::max(maxX, x);
      strokes.back().push_back({x, baseY + point.warpMs * pxPerMs,
                                baseY + (point.warpMs + point.errorMs) *
                                            pxPerMs,
                                point.errorMs});
    }
    if (minX > maxX)
      continue; // nothing judged yet (spline still warming up)

    // The zero line: the constant-tempo reference.
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(withAlpha(ink, 0.3), 0.1 * sp, Qt::DashLine));
    painter.drawLine(QPointF(std::max(minX, viewport.left()), baseY),
                     QPointF(std::min(maxX, viewport.right()), baseY));

    for (const auto &stroke : strokes)
    {
      // The fitted curve, smoothed through the genuine spline values
      // (Catmull-Rom control points).
      if (stroke.size() >= 2)
      {
        QPainterPath path(QPointF(stroke.front().x, stroke.front().curveY));
        for (std::size_t i = 1; i < stroke.size(); ++i)
        {
          const Node &n0 = stroke[i > 1 ? i - 2 : 0];
          const Node &n1 = stroke[i - 1];
          const Node &n2 = stroke[i];
          const Node &n3 = stroke[i + 1 < stroke.size() ? i + 1 : i];
          path.cubicTo(QPointF(n1.x + (n2.x - n0.x) / 6.0,
                               n1.curveY + (n2.curveY - n0.curveY) / 6.0),
                       QPointF(n2.x - (n3.x - n1.x) / 6.0,
                               n2.curveY - (n3.curveY - n1.curveY) / 6.0),
                       QPointF(n2.x, n2.curveY));
        }
        painter.setPen(QPen(withAlpha(ink, 0.6), 0.15 * sp));
        painter.drawPath(path);
      }

      // The onsets: a dot at each actual deviation, and the residual — the
      // early/late verdict — as its connector down/up to the curve.
      for (const Node &node : stroke)
      {
        if (node.x < viewport.left() || node.x > viewport.right())
          continue;
        painter.setPen(QPen(withAlpha(ink, 0.4), 0.1 * sp));
        painter.drawLine(QPointF(node.x, node.curveY),
                         QPointF(node.x, node.dotY));
        painter.setPen(Qt::NoPen);
        painter.setBrush(withAlpha(errorColor(node.errorMs), 0.95));
        painter.drawEllipse(QPointF(node.x, node.dotY), 0.35 * sp, 0.35 * sp);
      }
    }

    // The adaptive scale, labelled at the band's visible left end.
    QFont font = painter.font();
    font.setPixelSize(std::max(1, static_cast<int>(1.6 * sp)));
    painter.setFont(font);
    painter.setPen(withAlpha(ink, 0.7));
    painter.drawText(QPointF(std::max(minX, viewport.left()) + sp,
                             baseY - (halfLenSp + 0.6) * sp),
                     QString::fromUtf8("±%1 ms").arg(qRound(maxAbsMs)));
  }
}

void TimingFeedbackOverlay::paintGauge(QPainter &painter, const Gauge &gauge,
                                       double anchorX, double opacity) const
{
  const double sp = gauge.spatium;
  const double x = anchorX;
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

  // The dynamics marker shares the stem: a ring on its own scale — high =
  // too loud, low = too soft, pinned at the ends beyond ±dynamicsRange — in
  // a darker, translucent shade of its accuracy colour. Dead-on timing and
  // dynamics nest the dot inside the ring: one bigger circle.
  if (gauge.dynamicsError)
  {
    const double error = *gauge.dynamicsError;
    const double y =
        centerY - std::clamp(error / dynamicsRange, -1.0, 1.0) * halfLenSp * sp;
    painter.setBrush(Qt::NoBrush);
    painter.setPen(
        QPen(withAlpha(ratioColor(std::abs(error) / dynamicsRange).darker(135),
                       0.85 * opacity),
             0.22 * sp));
    painter.drawEllipse(QPointF(x, y), ringRadiusSp * sp, ringRadiusSp * sp);
  }
}

std::vector<std::pair<double, double>>
TimingFeedbackOverlay::recentSignedSamples(const SampleMap &samples) const
{
  // The in-window samples (at their latest revised errors) with their recency
  // weights: an old note counts for little, the last few notes dominate the
  // statistics.
  const qint64 now = _clock.elapsed();
  std::vector<std::pair<double /*error*/, double /*weight*/>> data;
  for (const auto &[staff, staffSamples] : samples)
    for (const auto &[onsetTMs, sample] : staffSamples)
    {
      const double age = static_cast<double>(now - sample.arrivalMs);
      if (age <= sampleWindowMs)
        data.emplace_back(sample.errorMs, std::exp(-age / recencyTauMs));
    }
  return data;
}

void TimingFeedbackOverlay::paintBoxPlots(QPainter &painter,
                                          const QRectF &viewport,
                                          double scaling) const
{
  // The main (timing) panel, bottom-right, hosting the combined-score band —
  // the same composition the final banner reports, but recency-weighted —
  // with the per-component breakdown (hidden when only one component has
  // samples: it would just repeat the combined score).
  const std::vector<Component> components{
      {"tempo", recentQuantile(_samples), scoreRefMs},
      {"sync", recentSyncAbsErrorQuantile(), scoreRefMs},
      {"dyn", recentQuantile(_dynamicsSamples), dynamicsScoreRef}};
  std::optional<ScoreBandSpec> combinedBand;
  if (const auto composite = combineScores(components))
    combinedBand =
        ScoreBandSpec{QStringLiteral("score"), composite->score,
                      composite->meanErrorRatio, breakdownText(components)};
  paintBoxPlotPanel(painter, viewport, scaling, recentSignedSamples(_samples),
                    false, rangeMs, "early", "late",
                    QString::fromUtf8("±%1 ms").arg(rangeMs), combinedBand);

  // The dynamics panel, bottom-left, with its own sub-score. Negated so
  // "too loud" reads upward, matching the gauges' ring.
  std::optional<ScoreBandSpec> dynamicsBand;
  if (const auto quantile = recentQuantile(_dynamicsSamples))
    dynamicsBand = ScoreBandSpec{QStringLiteral("dyn score"),
                                 scoreOf(*quantile, dynamicsScoreRef),
                                 *quantile / dynamicsScoreRef, QString()};
  auto dynamics = recentSignedSamples(_dynamicsSamples);
  for (auto &[error, weight] : dynamics)
    error = -error;
  paintBoxPlotPanel(painter, viewport, scaling, std::move(dynamics), true,
                    dynamicsRange, "loud", "soft",
                    QString::fromUtf8("±%1 %").arg(dynamicsRange * 100.0),
                    dynamicsBand);
}

void TimingFeedbackOverlay::paintBoxPlotPanel(
    QPainter &painter, const QRectF &viewport, double scaling,
    std::vector<std::pair<double, double>> data, bool dockLeft, double range,
    const char *topLabel, const char *bottomLabel, const QString &rangeLabel,
    const std::optional<ScoreBandSpec> &scoreBand) const
{
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

  // Dock in the viewport's bottom corner at constant physical size: translate
  // to the panel's logical origin, then undo the zoom so everything below is
  // in physical px.
  const double bandPx = scoreBand ? scoreBandPx : 0.0;
  const double panelHeight = bandPx + hudHeightPx;
  painter.save();
  painter.translate(dockLeft ? viewport.left() + hudMarginPx / scaling
                             : viewport.right() -
                                   (hudWidthPx + hudMarginPx) / scaling,
                    viewport.bottom() - (panelHeight + hudMarginPx) / scaling);
  painter.scale(1.0 / scaling, 1.0 / scaling);

  painter.setPen(Qt::NoPen);
  painter.setBrush(QColor(0, 0, 0, 130));
  painter.drawRoundedRect(QRectF(0, 0, hudWidthPx, panelHeight), 8, 8);

  // Beyond-scale values get pinned into a small band past the ± range
  // limits. The score band, if any, sits above the plot.
  const double plotTop = bandPx + hudPaddingPx + hudLabelBandPx;
  const double plotBottom = panelHeight - hudPaddingPx - hudLabelBandPx;
  const double plotLeft = hudPaddingPx;
  const double plotRight = hudWidthPx - hudPaddingPx;
  const double limitTop = plotTop + outlierBandPx;
  const double limitBottom = plotBottom - outlierBandPx;
  const auto yOf = [&](double value)
  {
    if (value < -range)
      return plotTop + 0.5 * outlierBandPx;
    if (value > range)
      return plotBottom - 0.5 * outlierBandPx;
    return limitTop +
           (value + range) / (2.0 * range) * (limitBottom - limitTop);
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
  const QColor color = ratioColor(std::abs(median) / range);
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
    painter.setBrush(
        withAlpha(ratioColor(std::abs(outliers[i]) / range), 0.85));
    const double x = cx + (static_cast<double>(i % 5) - 2.0) * 4.0;
    painter.drawEllipse(QPointF(x, yOf(outliers[i])), 2.0, 2.0);
  }

  QFont font = painter.font();
  font.setPixelSize(9);
  painter.setFont(font);
  painter.setPen(QColor(235, 235, 235, 220));
  painter.drawText(QRectF(plotLeft, bandPx + 1, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx),
                   Qt::AlignLeft | Qt::AlignVCenter, topLabel);
  painter.drawText(QRectF(plotLeft, bandPx + 1, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString("n=%1").arg(static_cast<int>(data.size())));
  painter.drawText(QRectF(plotLeft, plotBottom, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx - 1),
                   Qt::AlignLeft | Qt::AlignVCenter, bottomLabel);
  painter.drawText(QRectF(plotLeft, plotBottom, plotRight - plotLeft,
                          hudLabelBandPx + hudPaddingPx - 1),
                   Qt::AlignRight | Qt::AlignVCenter, rangeLabel);

  // The score band: the panel's score, big, in its accuracy colour, with an
  // optional breakdown line beneath.
  if (scoreBand)
  {
    painter.drawText(QRectF(plotLeft, 0, plotRight - plotLeft, scoreMainRowPx),
                     Qt::AlignLeft | Qt::AlignVCenter, scoreBand->label);
    painter.drawText(QRectF(plotLeft, scoreMainRowPx - 4, plotRight - plotLeft,
                            scoreBandPx - scoreMainRowPx + 4),
                     Qt::AlignLeft | Qt::AlignVCenter, scoreBand->breakdown);
    QFont scoreFont = painter.font();
    scoreFont.setPixelSize(26);
    scoreFont.setBold(true);
    painter.setFont(scoreFont);
    // Colour by the error relative to the component references, expressed on
    // the gauge's ms scale.
    painter.setPen(
        withAlpha(errorColor(scoreBand->errorRatio * scoreRefMs), 1.0));
    painter.drawText(QRectF(plotLeft, 0, plotRight - plotLeft, scoreMainRowPx),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QString::number(scoreBand->score));
  }
  painter.restore();
}
} // namespace dgk
