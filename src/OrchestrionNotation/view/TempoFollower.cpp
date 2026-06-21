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

namespace dgk
{
namespace
{
// The leading note rests mid-viewport; a lagging onset must stay this many
// physical pixels clear of the left edge to count as "fitting".
constexpr double playheadFrac = 0.5;
constexpr double edgeMarginPx = 48.0;
// Zoom-easing time constant (s): the zoom adapts gently, never snappily.
constexpr double tauZoom = 0.35;
} // namespace

TempoFollower::TempoFollower(Canvas &canvas) : _canvas(canvas)
{
  _clock.start();
  _timer.setInterval(16); // ~60 fps
  _timer.callOnTimeout([this] { tick(); });
}

void TempoFollower::onOnsets(const std::map<int, double> &presentOnsets,
                             std::optional<double> leadingAny,
                             std::optional<double> trailingAny)
{
  if (_suspended)
  {
    // A manual click/swipe suspended us; resume only when a note is actually
    // played again. Start fresh so we re-frame at the current position and
    // rebuild the tempo estimates (the timestamps from while paused are stale).
    if (presentOnsets.empty())
      return;
    reset();
  }

  // One-shot framing once we have a laid-out viewport and a position.
  if (!_framed && leadingAny && _canvas.viewWidth() > 1.0)
    frame(*leadingAny, trailingAny.value_or(*leadingAny));

  // Feed each sounding hand's tracker a tempo observation.
  bool observed = false;
  const double now = static_cast<double>(_clock.elapsed());
  for (const auto &[track, onsetX] : presentOnsets)
  {
    Hand &hand = _hands[track];
    if (onsetX == hand.lastOnsetX)
      continue;
    // A repeat replays earlier bars: this hand's onset x jumps backward with no
    // position-jump signal. Fold each backward jump into the hand's offset so
    // its tracked coordinate stays monotonic (the tempo carries straight
    // through), and subtract it again for display so the view snaps back.
    if (!std::isnan(hand.lastOnsetX) && onsetX < hand.lastOnsetX)
      hand.xOffset += hand.lastOnsetX - onsetX;
    hand.lastOnsetX = onsetX;
    hand.tracker.addObservation(now, onsetX + hand.xOffset);
    observed = true;
  }
  if (observed && !_timer.isActive())
    _timer.start();
}

void TempoFollower::frame(double leadingX, double trailingX)
{
  const double userScale = _canvas.defaultScaling();
  double scale = userScale;
  if (trailingX < leadingX)
  {
    // Zoom out (never in) so the trailing onset fits to the left of the
    // centered leading onset, past a small edge margin.
    const double availLeftPx =
        playheadFrac * _canvas.viewWidth() - edgeMarginPx;
    const double spanLogical = leadingX - trailingX;
    if (availLeftPx > 0.0 && spanLogical > 1e-6)
      scale = std::min(userScale, availLeftPx / spanLogical);
    scale = std::clamp(scale, _canvas.minScaling(), userScale);
  }
  _scaling = scale;
  _canvas.centerOn(leadingX, scale);
  _framed = true;
}

void TempoFollower::tick()
{
  const double now = static_cast<double>(_clock.elapsed());

  // Advance each hand's tracker (coasting to a stop if its next note is
  // overdue), and collect the on-screen layout positions: the leading
  // (rightmost) hand is centered; the leftmost *actively-playing* hand sets how
  // far we must zoom out to keep it in view.
  std::optional<double> leadingX;
  std::optional<double> trailingActiveX;
  bool allCoasting = true;
  for (auto &[track, hand] : _hands)
  {
    hand.tracker.heartbeat(now);
    if (!hand.tracker.ready())
      continue;
    const double pos = hand.tracker.positionAt(now) - hand.xOffset;
    leadingX = leadingX ? std::max(*leadingX, pos) : pos;
    if (!hand.tracker.isCoasting())
    {
      allCoasting = false;
      trailingActiveX = trailingActiveX ? std::min(*trailingActiveX, pos) : pos;
    }
  }
  if (!leadingX)
    return;

  // Zoom as far in as the user's default allows, but far enough out that a
  // lagging active hand stays just inside the left edge.
  const double userScale = _canvas.defaultScaling();
  double targetScale = userScale;
  if (trailingActiveX && *trailingActiveX < *leadingX)
  {
    const double availLeftPx = playheadFrac * _canvas.viewWidth() - edgeMarginPx;
    const double spanLogical = *leadingX - *trailingActiveX;
    if (availLeftPx > 0.0 && spanLogical > 1e-6)
      targetScale = std::min(userScale, availLeftPx / spanLogical);
    targetScale = std::clamp(targetScale, _canvas.minScaling(), userScale);
  }

  // Ease the zoom gently (the pan is already smooth via the trackers).
  if (_scaling <= 0.0)
    _scaling = targetScale;
  else
  {
    const double dt = _lastTickMs > 0 ? (now - _lastTickMs) / 1000.0 : 0.016;
    _scaling += (targetScale - _scaling) * (1.0 - std::exp(-dt / tauZoom));
  }
  _lastTickMs = static_cast<qint64>(now);

  _canvas.centerOn(*leadingX, _scaling);

  // Once every hand has coasted to a stop and the zoom has settled there is
  // nothing left to animate: idle until the next note restarts the timer.
  // (Gated on all-coasting so a live glide is never cut short.)
  const bool settled = std::isfinite(_lastLeadingX) &&
                       std::abs(*leadingX - _lastLeadingX) < 0.02 &&
                       std::abs(targetScale - _scaling) < 1e-4;
  _lastLeadingX = *leadingX;
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
