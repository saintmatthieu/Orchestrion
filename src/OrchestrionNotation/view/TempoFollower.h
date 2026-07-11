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

  //! A sounding onset: its page-logical x (drives the scroll), its
  //! playback-unrolled tick — continuous through repeats, voltas and jumps —
  //! which drives the musical-tempo readout and the timing judgments, and the
  //! gesture's raw controller velocity (0..1) when the input device measures
  //! one, driving the dynamics judgments.
  struct Onset
  {
    double x;
    double tick;
    std::optional<double> velocity;
  };

  //! Verdict on one onset's timing, measured against its hand's *smoothed*
  //! tempo curve (the spline) — the atomic signal of the performance game.
  //! The spline bends with the performer, so a smooth tempo shape (rubato, a
  //! ritardando) is not penalized; only deviation from the performer's own
  //! smooth curve is. Since the spline is re-fitted on every onset, the same
  //! onset — identified by \p tMs — is re-judged as it gets hindsight, until
  //! it leaves the smoothing window.
  struct Judgment
  {
    double tMs;     // the onset's time (this class's clock): its identity
    double errorMs; // signed arrival error: − = early, + = late
    //! For the time-proportional overlays, in playback ticks: how far the
    //! fitted smooth timeline displaces this note from the window's
    //! constant-tempo reference (+ = later/right), and the actual arrival's
    //! additional displacement from the fitted one. Both re-fitted with the
    //! spline; unused by the dynamics judgments.
    double warpTicks = 0.0;
    double errorTicks = 0.0;
    //! The same fitted displacement in milliseconds, for the deviation
    //! ribbon (the note's actual deviation is warpMs + errorMs).
    double warpMs = 0.0;
    //! The fitted tempo at this onset, in BPM (for the tooltips); re-fitted
    //! like the rest. 0 for the dynamics judgments.
    double bpm = 0.0;
  };

  //! What one transition batch yields for the performance game.
  struct Feedback
  {
    //! Per sounding hand, the (re-)judgments of *all* its onsets still in
    //! the smoothing window, newest last — empty while the spline is warming
    //! up (at the start, or on resuming after a stop).
    std::map<int, std::vector<Judgment>> judgments;
    //! Same shape for the dynamics: each onset's played velocity measured
    //! against the hand's smoothed loudness curve. Here Judgment::errorMs
    //! actually holds a velocity-fraction error (0..1 scale, + = louder than
    //! the curve). Only gestures with a real controller velocity contribute.
    std::map<int, std::vector<Judgment>> dynamicsJudgments;
    //! One hand-asynchrony sample, when at least two hands are playing: how
    //! far apart the hands' smoothed position curves are at a recent, settled
    //! instant, converted to time (+ = the upper staff leads). Absent for
    //! single-staff scores or while either spline is warming up or coasting.
    std::optional<Judgment> handSync;
  };

  //! Feed one transition batch. \p presentOnsets maps each hand (staff) that is
  //! *sounding* this batch to its onset — a tempo observation for that hand.
  //! \p leadingAny / \p trailingAny are the rightmost / leftmost onset x that
  //! is sounding *or* upcoming, used once to frame the start.
  Feedback onOnsets(const std::map<int, Onset> &presentOnsets,
                    std::optional<double> leadingAny,
                    std::optional<double> trailingAny);

  //! Tempo-following auto-play. While \p staff is set, that hand belongs to
  //! the machine: it is exempt from judgments, dynamics and sync sampling,
  //! and whenever the *manual* hands' estimated position reaches one of its
  //! targets (see setAutoTargets), \p fire is invoked — noteOn to strike its
  //! next chord, noteOff to release the current one. The auto hand thereby
  //! follows the performer, accelerating, slowing and coasting to a stop with
  //! them. \p fire is called from the follow tick: defer any re-entrant work.
  void setAutoPlay(std::optional<int> staff,
                   std::function<void(bool noteOn)> fire);

  //! The auto hand's due points, in playback-unrolled ticks: the sounding
  //! chord's end (→ noteOff) and the next chord's begin (→ noteOn). The owner
  //! refreshes them from the sequencer's transitions after every batch; each
  //! fires at most once until refreshed.
  void setAutoTargets(std::optional<double> offTick,
                      std::optional<double> onTick);

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
    // Musical tempo, tracked in playback-unrolled ticks (continuous through
    // repeats and jumps) — same model, so the visualization's BPM readout is
    // *fitted* across onsets (smoothing per-onset jitter) rather than a raw
    // single-interval ratio, and is independent of layout spacing. Causal
    // trace + re-smoothed spline.
    TempoTracker tempoTracker;
    TempoSmoother tempoSmoother;
    // Played loudness, smoothed over real time: the same state-space model
    // fits a smoothing spline to whatever it observes, here the gestures'
    // controller velocities — so a musical swell is part of the curve and
    // only erratic loudness registers as error.
    TempoSmoother dynamicsSmoother;
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

  // Tempo-following auto-play (see setAutoPlay / setAutoTargets).
  std::optional<int> _autoStaff;
  std::function<void(bool)> _autoFire;
  std::optional<double> _autoOffTick;
  std::optional<double> _autoOnTick;
};
} // namespace dgk
