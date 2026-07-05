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

#include "TempoTracking/TempoSmoother.h"
#include "TempoTracking/TempoTracker.h"

#include <QElapsedTimer>
#include <QTimer>

#include <limits>
#include <map>
#include <optional>
#include <vector>

namespace dgk
{
//! Drives a constant-speed score scroll from played onsets. It runs one
//! TempoTracker + TempoSmoother per hand (= staff; the owner keys onsets by
//! staff, since the voices on a staff share the same gestures), so each hand
//! keeps its own tempo. A ~60 fps timer keeps the *leading* hand's scroll
//! anchor at the playhead (the first third of the view): the anchor is the
//! *smoothed* (spline) position a couple of onsets back — steadier than the
//! causal extrapolation, whose delay the off-center playhead leaves room for
//! (the notes being played float around mid-view, ahead of the anchor). A
//! *trailing* hand that is actively playing behind pulls the zoom out (only as
//! far as needed) so it stays in view, while a trailing hand that has gone
//! quiet is allowed to slip out. Zoom stays as far in as the user's default
//! allows.
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

  //! Optional debug tap: receives the model's per-frame state and the onsets it
  //! reacts to, for a real-time visualization. Kept Qt-free (like Canvas).
  class VizSink
  {
  public:
    virtual ~VizSink() = default;
    struct HandTempo
    {
      int staff;
      double tempo;  // estimated tempo (musical units / ms)
      bool coasting; // next onset overdue: extrapolating, not tracking live
    };
    //! Per follow-tick: the current tempo of each ready hand at time \p tMs.
    virtual void onTempoSample(double tMs,
                               const std::vector<HandTempo> &hands) = 0;
    //! A hand played an onset (the model's input) at time \p tMs.
    virtual void onOnset(double tMs, int staff) = 0;
    struct CurvePoint
    {
      double tMs;
      double tempo;
    };
    //! Per onset: one hand's re-smoothed tempo curve (the spline the causal
    //! per-frame estimate only chases), sampled over the smoothing window.
    //! Replaces that hand's previous curve.
    virtual void onSmoothedTempo(int staff,
                                 const std::vector<CurvePoint> &curve) = 0;
  };

  //! Where the scroll anchor rests horizontally in the view. Off-center (first
  //! third) because the anchor trails the notes being played by the smoothing
  //! delay — the playhead itself floats in the two thirds ahead of it.
  static constexpr double anchorFrac = 1.0 / 3.0;

  explicit TempoFollower(Canvas &canvas, VizSink *viz = nullptr);

  TempoFollower(const TempoFollower &) = delete;
  TempoFollower &operator=(const TempoFollower &) = delete;

  //! A sounding onset: its page-logical x (drives the scroll) and its score
  //! tick (drives the musical-tempo readout for the visualization).
  struct Onset
  {
    double x;
    double tick;
  };

  //! Feed one transition batch. \p presentOnsets maps each hand (staff) that is
  //! *sounding* this batch to its onset — a tempo observation for that hand.
  //! \p leadingAny / \p trailingAny are the rightmost / leftmost onset x that is
  //! sounding *or* upcoming, used once to frame the start.
  void onOnsets(const std::map<int, Onset> &presentOnsets,
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
  //! Per-hand state: its own tempo trackers/smoothers, plus repeat
  //! bookkeeping. They work in a repeat-unrolled coordinate (onset x +
  //! xOffset, kept monotonic); the on-screen layout x is positionAt() −
  //! xOffset.
  struct Hand
  {
    // Scroll position, tracked in page-logical x: the causal filter (real-time
    // "now" and coasting) …
    TempoTracker tracker;
    // … and the smoothed spline it chases, which the scroll anchors on (a
    // couple of onsets delayed, but with a continuous velocity).
    TempoSmoother smoother;
    // Musical tempo, tracked in score ticks — same model, so the
    // visualization's BPM readout is *fitted* across onsets (smoothing
    // per-onset jitter) rather than a raw single-interval ratio, and is
    // independent of layout spacing. Causal trace + re-smoothed spline.
    TempoTracker tempoTracker;
    TempoSmoother tempoSmoother;
    // Accumulated leftward jump of repeated bars for this hand: added to its
    // observations to keep them monotonic, subtracted again for display.
    double xOffset = 0.0;
    double lastOnsetX = std::numeric_limits<double>::quiet_NaN();
    // Scroll-anchor continuity (see anchorAt): the raw anchor jumps a little
    // at each onset; the jump is folded into a decaying offset instead.
    double lastRawAnchor = std::numeric_limits<double>::quiet_NaN();
    double lastAnchorTime = 0.0;
    double anchorOffset = 0.0;
    bool anchorDirty = false;
  };

  void tick();
  void frame(double leadingX, double trailingX);
  double anchorAt(Hand &hand, double now);

  Canvas &_canvas;
  VizSink *_viz; // optional, not owned
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
