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

namespace dgk
{
namespace
{
// The leading note rests mid-viewport; a lagging onset must stay this many
// physical pixels clear of the left edge for the initial framing to fit it.
constexpr double playheadFrac = 0.5;
constexpr double edgeMarginPx = 48.0;
} // namespace

TempoFollower::TempoFollower(Canvas &canvas) : _canvas(canvas)
{
  _clock.start();
  _timer.setInterval(16); // ~60 fps
  _timer.callOnTimeout([this] { tick(); });
}

void TempoFollower::onOnsets(std::optional<double> leadingAny,
                             std::optional<double> trailingAny,
                             std::optional<double> leadingPresent)
{
  // One-shot framing once we have a laid-out viewport and a position.
  if (!_framed && leadingAny && _canvas.viewWidth() > 1.0)
    frame(*leadingAny, trailingAny.value_or(*leadingAny));

  // A distinct sounding onset is a tempo observation; keep the timer running
  // while playing.
  if (leadingPresent && *leadingPresent != _lastOnsetX)
  {
    _lastOnsetX = *leadingPresent;
    _tracker.addObservation(static_cast<double>(_clock.elapsed()),
                            *leadingPresent);
    if (!_timer.isActive())
      _timer.start();
  }
}

void TempoFollower::frame(double leadingX, double trailingX)
{
  const double userScale = _canvas.viewScaling();
  double scale = userScale;
  if (trailingX < leadingX)
  {
    // Zoom out (never in) so the trailing onset fits to the left of the
    // centered leading onset, past a small edge margin.
    const double availLeftPx = playheadFrac * _canvas.viewWidth() - edgeMarginPx;
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
  const double scaling = _scaling > 0.0 ? _scaling : _canvas.viewScaling();
  const double x =
      _tracker.positionAt(static_cast<double>(_clock.elapsed()));
  _canvas.centerOn(x, scaling);
}

void TempoFollower::reset()
{
  _tracker.reset();
  _timer.stop();
  _framed = false;
  _scaling = 0.0;
  _lastOnsetX = std::numeric_limits<double>::quiet_NaN();
}
} // namespace dgk
