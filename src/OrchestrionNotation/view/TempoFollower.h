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
#include <map>
#include <optional>

namespace dgk
{
//! Drives a constant-speed score scroll from played onsets. It runs one
//! TempoTracker per hand (= staff; the owner keys onsets by staff, since the
//! voices on a staff share the same gestures), so each hand keeps its own
//! tempo. A ~60 fps timer keeps the *leading* hand's extrapolated position at
//! the playhead; a *trailing* hand that is actively playing behind it pulls the
//! zoom out (only as far as needed) so it stays in view, while a trailing hand
//! that has gone quiet is allowed to slip out. Zoom stays as far in as the
//! user's default allows.
//!
//! The owner extracts per-track onset x-positions (page-logical) from playback
//! transitions and feeds them in. It knows nothing about the score model or the
//! Qt canvas: all viewport geometry and movement go through the Canvas
//! interface the owner implements (mirroring HighlightFader / KineticScroller).
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
    //! The user's chosen zoom — the most zoomed-in the auto-zoom will ever go.
    virtual double defaultScaling() const = 0;
    //! Smallest zoom the auto zoom-out may reach.
    virtual double minScaling() const = 0;
    //! Place logical x \p logicalX at the horizontal playhead at \p scaling,
    //! keeping the system vertically centered, and request a repaint.
    virtual void centerOn(double logicalX, double scaling) = 0;
  };

  explicit TempoFollower(Canvas &canvas);

  TempoFollower(const TempoFollower &) = delete;
  TempoFollower &operator=(const TempoFollower &) = delete;

  //! Feed one transition batch, all in page-logical x:
  //! \p presentOnsets maps each hand (staff) that is *sounding* this batch to
  //! its onset x — a tempo observation for that hand. \p leadingAny /
  //! \p trailingAny are the rightmost / leftmost onset that is sounding *or*
  //! upcoming, used once to frame the start.
  void onOnsets(const std::map<int, double> &presentOnsets,
                std::optional<double> leadingAny,
                std::optional<double> trailingAny);

  //! Stop following and hand control back to the user — a "panic" for a manual
  //! click or swipe. Stays suspended until the next played note (which resumes
  //! following fresh) or a reset().
  void suspend();

  //! Forget the tempo estimate and framing and stop following (e.g. a position
  //! jump or a new score). Also clears a suspend(), re-arming the follower.
  void reset();

private:
  void tick();
  void frame(double leadingX, double trailingX);

  //! Per-hand state: its own tempo tracker, plus repeat bookkeeping. The
  //! tracker works in a repeat-unrolled coordinate (onset x + xOffset, kept
  //! monotonic); the on-screen layout x is positionAt() − xOffset.
  struct Hand
  {
    TempoTracker tracker;
    // Accumulated leftward jump of repeated bars for this hand: added to its
    // observations to keep them monotonic, subtracted again for display.
    double xOffset = 0.0;
    double lastOnsetX = std::numeric_limits<double>::quiet_NaN();
  };

  Canvas &_canvas;
  std::map<int /*staff*/, Hand> _hands;
  QElapsedTimer _clock; // wall clock for observations (ms)
  QTimer _timer;        // ~60 fps follow tick
  bool _framed = false;
  bool _suspended =
      false;              // user took manual control; ignore onsets until reset
  double _scaling = 0.0;  // current (eased) zoom; 0 = unset
  qint64 _lastTickMs = 0; // for zoom-easing dt
  // Last centered position, to detect when the coast has settled (then idle).
  double _lastLeadingX = std::numeric_limits<double>::quiet_NaN();
};
} // namespace dgk
