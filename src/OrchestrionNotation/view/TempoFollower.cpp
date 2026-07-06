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
#include "TempoFollower.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace dgk
{
namespace
{
// An onset must stay this many physical pixels clear of the view edges to
// count as "fitting".
constexpr double edgeMarginPx = 48.0;
// Zoom-easing time constant (s): the zoom adapts gently, never snappily.
constexpr double tauZoom = 0.35;
// Score ticks per quarter note (MuseScore's division), to express the
// visualization's tempo readout in BPM.
constexpr double ticksPerQuarter = 480.0;
// Timing-judgment windows: an onset within max(floor, frac·cadence) of its
// predicted time is Perfect / Good. The fraction scales the tolerance to the
// playing cadence (a 30 ms slip on a whole note is not the sin it is on a
// sixteenth); the ms floor keeps fast passages humanly attainable.
constexpr double perfectFloorMs = 20.0;
constexpr double perfectFrac = 0.06;
constexpr double goodFloorMs = 45.0;
constexpr double goodFrac = 0.15;
// No judgment until this many onsets have re-fed a hand's estimate: right
// after the two-onset seed the curve fits the performer by construction, so
// early residuals are flattery, not information.
constexpr int judgeWarmupObservations = 3;

// The scroll anchors on the smoothed position this many onsets back: enough
// that the spline there is essentially settled (each onset's influence decays
// by the memory factor per knot), small enough that the playhead ahead of the
// anchor stays comfortably in view.
constexpr double smoothDelayIntervals = 4.0;

// Decay time constant (ms) for the offset that absorbs the small jump the
// anchor makes at each onset (the spline re-fit plus the delay's cadence
// update), keeping the displayed anchor continuous — same trick as the
// tracker's own continuity offset.
constexpr double anchorOffsetDecayMs = 1000.0;

// The smoothed tempo curve pushed to the visualization, as a dense polyline
// (the view stays ignorant of the spline's cubic segments).
std::vector<TempoFollower::VizSink::CurvePoint>
sampleSmoothedBpm(const TempoSmoother &smoother)
{
  constexpr double stepMs = 40.0;
  const double tBegin = smoother.knots().front().time;
  const double tEnd = smoother.knots().back().time;
  std::vector<TempoFollower::VizSink::CurvePoint> curve;
  curve.reserve(static_cast<std::size_t>((tEnd - tBegin) / stepMs) + 2);
  for (double t = tBegin; t < tEnd; t += stepMs)
    curve.push_back({t, smoother.velocityAt(t) * (60000.0 / ticksPerQuarter)});
  curve.push_back(
      {tEnd, smoother.velocityAt(tEnd) * (60000.0 / ticksPerQuarter)});
  return curve;
}
} // namespace

TempoFollower::TempoFollower(Canvas &canvas, VizSink *viz)
    : _canvas(canvas), _viz(viz)
{
  _clock.start();
  _timer.setInterval(16); // ~60 fps
  _timer.callOnTimeout([this] { tick(); });
}

std::map<int, TempoFollower::Judgment>
TempoFollower::onOnsets(const std::map<int, Onset> &presentOnsets,
                        std::optional<double> leadingAny,
                        std::optional<double> trailingAny)
{
  std::map<int, Judgment> judgments;
  if (_suspended)
  {
    // A manual click/swipe suspended us; resume only when a note is actually
    // played again. Start fresh so we re-frame at the current position and
    // rebuild the tempo estimates (the timestamps from while paused are stale).
    if (presentOnsets.empty())
      return judgments;
    reset();
  }

  // One-shot framing once we have a laid-out viewport and a position.
  if (!_framed && leadingAny && _canvas.viewWidth() > 1.0)
    frame(*leadingAny, trailingAny.value_or(*leadingAny));

  // Feed each sounding hand's tracker a tempo observation.
  bool observed = false;
  const double now = static_cast<double>(_clock.elapsed());
  for (const auto &[track, onset] : presentOnsets)
  {
    Hand &hand = _hands[track];
    const double onsetX = onset.x;
    if (onsetX == hand.lastOnsetX)
      continue;
    // A repeat replays earlier bars: this hand's onset x jumps backward with no
    // position-jump signal. Fold each backward jump into the hand's offset so
    // its tracked coordinate stays monotonic (the tempo carries straight
    // through), and subtract it again for display so the view snaps back.
    const bool repeat =
        !std::isnan(hand.lastOnsetX) && onsetX < hand.lastOnsetX;
    if (repeat)
      hand.xOffset += hand.lastOnsetX - onsetX;
    hand.lastOnsetX = onsetX;
    hand.tracker.addObservation(now, onsetX + hand.xOffset);
    hand.smoother.addObservation(now, onsetX + hand.xOffset);

    // Feed the musical-tempo estimators the playback-unrolled tick: unlike
    // the score tick it is continuous through repeats, voltas and jumps, so
    // the tempo estimate — and the judgments — carry straight across them.
    // Only a coast restarts them: the performer stopped, and the wound-down
    // curve says nothing about the resumed tempo (a fresh take).
    if (hand.tempoTracker.isCoasting())
    {
      hand.tempoTracker.reset();
      hand.tempoSmoother.reset();
    }
    if (repeat)
    {
      // The view should snap back over the repeated bars, not glide: clear the
      // anchor-continuity state instead of absorbing the jump.
      hand.anchorOffset = 0.0;
      hand.lastRawAnchor = std::numeric_limits<double>::quiet_NaN();
      hand.anchorDirty = false;
    }
    else
      hand.anchorDirty = true;

    // Judge the onset's timing against the fitted curve *before* this
    // observation corrects it — once the estimate has settled past seeding.
    if (hand.tempoTracker.observations() >= judgeWarmupObservations &&
        hand.tempoTracker.speed() > 0.0 && hand.tempoTracker.intervalMs() > 0.0)
    {
      // Residual in ticks, negated into an arrival-time error: a note whose
      // tick the curve hasn't reached yet arrived early (− ms).
      const double errorMs =
          -hand.tempoTracker.predictionError(now, onset.tick) /
          hand.tempoTracker.speed();
      const double cadence = hand.tempoTracker.intervalMs();
      const double absErr = std::abs(errorMs);
      Judgment::Tier tier;
      if (absErr <= std::max(perfectFloorMs, perfectFrac * cadence))
        tier = Judgment::Tier::Perfect;
      else if (absErr <= std::max(goodFloorMs, goodFrac * cadence))
        tier = Judgment::Tier::Good;
      else
        tier = errorMs < 0.0 ? Judgment::Tier::Early : Judgment::Tier::Late;
      judgments[track] = {tier, errorMs};
    }

    hand.tempoTracker.addObservation(now, onset.tick);
    hand.tempoSmoother.addObservation(now, onset.tick);

    observed = true;
    if (_viz)
    {
      _viz->onOnset(now, track);
      if (hand.tempoSmoother.ready())
        _viz->onSmoothedTempo(track, sampleSmoothedBpm(hand.tempoSmoother));
    }
  }
  if (observed && !_timer.isActive())
    _timer.start();
  return judgments;
}

void TempoFollower::frame(double leadingX, double trailingX)
{
  const double userScale = _canvas.defaultScaling();
  double scale = userScale;
  if (trailingX < leadingX)
  {
    // Zoom out (never in) so the trailing onset fits to the left of the
    // anchored leading onset, past a small edge margin.
    const double availLeftPx = anchorFrac * _canvas.viewWidth() - edgeMarginPx;
    const double spanLogical = leadingX - trailingX;
    if (availLeftPx > 0.0 && spanLogical > 1e-6)
      scale = std::min(userScale, availLeftPx / spanLogical);
    scale = std::clamp(scale, _canvas.minScaling(), userScale);
  }
  _scaling = scale;
  _canvas.centerOn(leadingX, scale);
  _framed = true;
}

//! The scroll anchor for one hand: the smoothed (spline) position a few
//! onsets back — steadier than the causal extrapolation, at the cost of a
//! small fixed delay the off-center playhead leaves room for. Past the
//! spline's end (the performer has paused) it cross-fades to the causal,
//! coast-damped state evaluated at the same delayed time, so the anchor eases
//! to a stop with the tracker instead of extrapolating off the end.
//!
//! Between onsets the raw anchor moves continuously; at each onset it jumps a
//! little (the spline re-fit near its end, the delay's cadence update). The
//! jump is absorbed into a decaying offset so the *displayed* anchor carries
//! straight on and slides to the corrected trajectory gradually. A repeat's
//! intended backward snap is exempted (its continuity state is cleared).
double TempoFollower::anchorAt(Hand &hand, double now)
{
  double raw;
  double anchorSpeed;
  if (!hand.smoother.ready() || hand.tracker.intervalMs() <= 0.0)
  {
    raw = hand.tracker.positionAt(now) - hand.xOffset;
    anchorSpeed = hand.tracker.speed();
  }
  else
  {
    const double delayMs = smoothDelayIntervals * hand.tracker.intervalMs();
    const double tEval = now - delayMs;
    raw = hand.smoother.positionAt(tEval);
    anchorSpeed = hand.smoother.velocityAt(tEval);
    const double splineEnd = hand.smoother.knots().back().time;
    if (tEval > splineEnd)
    {
      const double w = std::min(1.0, (tEval - splineEnd) / delayMs);
      raw = (1.0 - w) * raw + w * hand.tracker.positionAt(tEval);
      anchorSpeed = (1.0 - w) * anchorSpeed + w * hand.tracker.speed();
    }
    raw -= hand.xOffset;
  }

  const bool hasLast = !std::isnan(hand.lastRawAnchor);
  const double dt = hasLast ? now - hand.lastAnchorTime : 0.0;
  if (dt > 0.0)
    hand.anchorOffset *= std::exp(-dt / anchorOffsetDecayMs);
  if (hand.anchorDirty)
  {
    // Where the previous trajectory would have carried the anchor by now (dt
    // capped: after an idle stretch there is no smooth motion to extend).
    if (hasLast)
      hand.anchorOffset +=
          hand.lastRawAnchor + anchorSpeed * std::min(dt, 50.0) - raw;
    hand.anchorDirty = false;
  }
  hand.lastRawAnchor = raw;
  hand.lastAnchorTime = now;
  return raw + hand.anchorOffset;
}

void TempoFollower::tick()
{
  const double now = static_cast<double>(_clock.elapsed());

  // Advance each hand's tracker (coasting to a stop if its next note is
  // overdue), and collect the on-screen layout positions: the leading
  // (rightmost) hand's smoothed, delayed anchor is what the view follows; the
  // causal "now" positions set how far we must zoom out — leftmost
  // *actively-playing* hand on the left side, the leading hand's own playhead
  // (running ahead of the anchor) on the right.
  std::optional<double> leadingAnchor;
  std::optional<double> leadingNow;
  std::optional<double> trailingActiveX;
  bool allCoasting = true;
  std::vector<VizSink::HandTempo> vizSamples;
  for (auto &[track, hand] : _hands)
  {
    hand.tracker.heartbeat(now);
    hand.tempoTracker.heartbeat(now);
    if (!hand.tracker.ready())
      continue;
    const double pos = hand.tracker.positionAt(now) - hand.xOffset;
    const double anchor = anchorAt(hand, now);
    leadingAnchor = leadingAnchor ? std::max(*leadingAnchor, anchor) : anchor;
    leadingNow = leadingNow ? std::max(*leadingNow, pos) : pos;
    const bool coasting = hand.tracker.isCoasting();
    if (!coasting)
    {
      allCoasting = false;
      trailingActiveX = trailingActiveX ? std::min(*trailingActiveX, pos) : pos;
    }
    if (_viz)
    {
      // Smoothed musical tempo, ticks/ms → BPM.
      const double bpm =
          hand.tempoTracker.speed() * (60000.0 / ticksPerQuarter);
      vizSamples.push_back({track, bpm, hand.tempoTracker.isCoasting()});
    }
  }
  if (_viz && !vizSamples.empty())
    _viz->onTempoSample(now, vizSamples);
  if (!leadingAnchor)
    return;

  // Zoom as far in as the user's default allows, but far enough out that a
  // lagging active hand stays just inside the left edge and the leading
  // playhead just inside the right edge.
  const double userScale = _canvas.defaultScaling();
  double targetScale = userScale;
  if (trailingActiveX && *trailingActiveX < *leadingAnchor)
  {
    const double availLeftPx = anchorFrac * _canvas.viewWidth() - edgeMarginPx;
    const double spanLogical = *leadingAnchor - *trailingActiveX;
    if (availLeftPx > 0.0 && spanLogical > 1e-6)
      targetScale = std::min(targetScale, availLeftPx / spanLogical);
  }
  if (leadingNow && *leadingNow > *leadingAnchor)
  {
    const double availRightPx =
        (1.0 - anchorFrac) * _canvas.viewWidth() - edgeMarginPx;
    const double spanLogical = *leadingNow - *leadingAnchor;
    if (availRightPx > 0.0 && spanLogical > 1e-6)
      targetScale = std::min(targetScale, availRightPx / spanLogical);
  }
  targetScale = std::clamp(targetScale, _canvas.minScaling(), userScale);

  // Ease the zoom gently (the pan is already smooth via the smoothers).
  if (_scaling <= 0.0)
    _scaling = targetScale;
  else
  {
    const double dt = _lastTickMs > 0 ? (now - _lastTickMs) / 1000.0 : 0.016;
    _scaling += (targetScale - _scaling) * (1.0 - std::exp(-dt / tauZoom));
  }
  _lastTickMs = static_cast<qint64>(now);

  _canvas.centerOn(*leadingAnchor, _scaling);

  // Once every hand has coasted to a stop and the zoom has settled there is
  // nothing left to animate: idle until the next note restarts the timer.
  // (Gated on all-coasting so a live glide is never cut short.)
  const bool settled = std::isfinite(_lastLeadingX) &&
                       std::abs(*leadingAnchor - _lastLeadingX) < 0.02 &&
                       std::abs(targetScale - _scaling) < 1e-4;
  _lastLeadingX = *leadingAnchor;
  if (allCoasting && settled)
    _timer.stop();
}

void TempoFollower::suspend()
{
  _suspended = true;
  _timer.stop();
}

void TempoFollower::reset()
{
  _hands.clear();
  _timer.stop();
  _framed = false;
  _suspended = false;
  _scaling = 0.0;
  _lastTickMs = 0;
  _lastLeadingX = std::numeric_limits<double>::quiet_NaN();
}
} // namespace dgk
