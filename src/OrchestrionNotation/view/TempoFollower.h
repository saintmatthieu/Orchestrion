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

#include "TempoTracking/TempoTracker.h"

#include <QElapsedTimer>
#include <QTimer>

#include <limits>
#include <optional>

namespace dgk
{
//! Drives a constant-speed score scroll from played onsets, using a
//! TempoTracker. The owner extracts onset x-positions (page-logical) from
//! playback transitions and feeds them in; a ~60 fps timer then keeps the
//! tracker's extrapolated position at the playhead, so a constant tempo yields
//! a constant scroll speed.
//!
//! It knows nothing about the score model or the Qt canvas: all viewport
//! geometry and movement go through the Canvas interface the owner implements
//! (mirroring how HighlightFader / KineticScroller take their owner's hooks).
class TempoFollower
{
public:
  //! Viewport queries and movement the follower needs from its owner. Method
  //! names are deliberately distinct from the paint view's own (width(),
  //! currentScaling(), …) so the view can implement this interface directly.
  class Canvas
  {
  public:
    virtual ~Canvas() = default;
    //! Viewport width in physical pixels.
    virtual double viewWidth() const = 0;
    //! Current zoom (physical pixels per logical unit).
    virtual double viewScaling() const = 0;
    //! Smallest zoom the auto zoom-out may reach.
    virtual double minScaling() const = 0;
    //! Place logical x \p logicalX at the horizontal playhead at \p scaling,
    //! keeping the system vertically centered, and request a repaint.
    virtual void centerOn(double logicalX, double scaling) = 0;
  };

  explicit TempoFollower(Canvas &canvas);

  TempoFollower(const TempoFollower &) = delete;
  TempoFollower &operator=(const TempoFollower &) = delete;

  //! Feed one transition batch's onset extents, all in page-logical x:
  //! \p leadingAny / \p trailingAny are the rightmost / leftmost onset that is
  //! sounding *or* upcoming (used once, to frame the start); \p leadingPresent
  //! is the rightmost *sounding* onset, recorded as a tempo observation.
  void onOnsets(std::optional<double> leadingAny,
                std::optional<double> trailingAny,
                std::optional<double> leadingPresent);

  //! Forget the tempo estimate and framing and stop following (e.g. a position
  //! jump or a new score).
  void reset();

private:
  void tick();
  void frame(double leadingX, double trailingX);

  Canvas &_canvas;
  TempoTracker _tracker;
  QElapsedTimer _clock; // wall clock for observations (ms)
  QTimer _timer;        // ~60 fps follow tick
  bool _framed = false;
  double _scaling = 0.0; // zoom chosen at framing and then held (0 = unset)
  double _lastOnsetX = std::numeric_limits<double>::quiet_NaN();
};
} // namespace dgk
