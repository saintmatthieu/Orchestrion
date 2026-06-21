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

void TempoFollower::onOnsets(std::optional<double> leadingAny,
                             std::optional<double> trailingAny,
                             std::optional<double> leadingPresent,
                             std::optional<double> trailingActive)
{
  if (_suspended)
  {
    // A manual click/swipe suspended us; resume only when a note is actually
    // played again. Start fresh so we re-frame at the current position and
    // rebuild the tempo estimate (the timestamps from while paused are stale).
    if (!leadingPresent)
      return;
    reset();
  }

  // Latest trailing-voice position drives the zoom (read each frame in tick()).
  _trailingX = trailingActive;

  // One-shot framing once we have a laid-out viewport and a position.
  if (!_framed && leadingAny && _canvas.viewWidth() > 1.0)
    frame(*leadingAny, trailingAny.value_or(*leadingAny));

  // A distinct sounding onset is a tempo observation; keep the timer running
  // while playing.
  if (leadingPresent && *leadingPresent != _lastOnsetX)
  {
    const double actualX = *leadingPresent;
    // A repeat replays earlier bars: the onset x jumps backward with no
    // position-jump signal. Fold each backward jump into an accumulating offset
    // so the tracked coordinate stays monotonic (the tempo estimate carries
    // straight through), and subtract the same offset for display so the view
    // snaps back to the repeated bars.
    if (!std::isnan(_lastOnsetX) && actualX < _lastOnsetX)
      _xOffset += _lastOnsetX - actualX;
    _lastOnsetX = actualX;
    _tracker.addObservation(static_cast<double>(_clock.elapsed()),
                            actualX + _xOffset);
    if (!_timer.isActive())
      _timer.start();
  }
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
  if (!_tracker.ready())
    return;

  const double now = static_cast<double>(_clock.elapsed());

  // Let the model decelerate if the next note is overdue (it coasts to a stop
  // rather than extrapolating off the end). The tracker works in the monotonic
  // (repeat-unrolled) coordinate; subtract the accumulated repeat offset to get
  // back to the on-screen layout x. This leading position is always centered.
  _tracker.heartbeat(now);
  const double leadingX = _tracker.positionAt(now) - _xOffset;

  // Zoom as far in as the user's default allows, but far enough out that the
  // trailing voice stays just inside the left edge.
  const double userScale = _canvas.defaultScaling();
  double targetScale = userScale;
  if (_trailingX && *_trailingX < leadingX)
  {
    const double availLeftPx = playheadFrac * _canvas.viewWidth() - edgeMarginPx;
    const double spanLogical = leadingX - *_trailingX;
    if (availLeftPx > 0.0 && spanLogical > 1e-6)
      targetScale = std::min(userScale, availLeftPx / spanLogical);
    targetScale = std::clamp(targetScale, _canvas.minScaling(), userScale);
  }

  // Ease the zoom gently (the pan is already smooth via the tracker).
  if (_scaling <= 0.0)
    _scaling = targetScale;
  else
  {
    const double dt = _lastTickMs > 0 ? (now - _lastTickMs) / 1000.0 : 0.016;
    _scaling += (targetScale - _scaling) * (1.0 - std::exp(-dt / tauZoom));
  }
  _lastTickMs = static_cast<qint64>(now);

  _canvas.centerOn(leadingX, _scaling);

  // Once the coast has eased to a stop and the zoom has settled there is nothing
  // left to animate: idle until the next note restarts the timer. (Gated on
  // coasting so a live glide is never cut short.)
  const bool settled = std::isfinite(_lastLeadingX) &&
                       std::abs(leadingX - _lastLeadingX) < 0.02 &&
                       std::abs(targetScale - _scaling) < 1e-4;
  _lastLeadingX = leadingX;
  if (_tracker.isCoasting() && settled)
    _timer.stop();
}

void TempoFollower::suspend()
{
  _suspended = true;
  _timer.stop();
}

void TempoFollower::reset()
{
  _tracker.reset();
  _timer.stop();
  _framed = false;
  _suspended = false;
  _scaling = 0.0;
  _lastTickMs = 0;
  _lastLeadingX = std::numeric_limits<double>::quiet_NaN();
  _trailingX.reset();
  _xOffset = 0.0;
  _lastOnsetX = std::numeric_limits<double>::quiet_NaN();
}
} // namespace dgk
