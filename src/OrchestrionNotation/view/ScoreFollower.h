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

#include <QElapsedTimer>
#include <QTimer>
#include <limits>
#include <map>
#include <optional>

namespace dgk
{
//! Keeps the played score in view while the performer plays it.
//!
//! The page does not follow the music continuously. Engraved x is not
//! proportional to time — in a "4 8 8" bar the quarter and the eighths get
//! nearly the same width — so a viewport pinned to the playing position must
//! speed up and slow down within every bar even for a metronomically perfect
//! performance. Instead the page stands still, and turns only when the next
//! event to play would drift past pageTriggerFrac of the width; that event is
//! then brought back to anchorFrac over a short ease. Between turns nothing
//! moves at all.
//!
//! The one exception to "the next event lands on the anchor" is the last turn
//! before a barrier — the barline past which the reading does not carry on:
//! a repeat's end on the pass that jumps back, or the final barline. What the
//! reader needs then is the way to the barrier and, for a repeat, the section
//! start it jumps back to; what precedes that start is past, and of no
//! interest. So the turn that would bring the barrier into view frames these
//! instead: it scrolls just far enough that the barrier is at least
//! barrierMarginPx inside the right edge *and* no more than barrierMarginPx
//! of what precedes the repeat's start shows at the left. Whichever of the
//! two asks for the further scroll decides — a long section leaves its start
//! off the page, a short one leaves more than the margin of what follows the
//! barrier on it, both fine — and when the start is on the page, the jump
//! back finds it there and the page does not move at all.
//!
//! The jump itself is anticipated. The last notes before it the performer can
//! hold in their fingers; the first notes after it they cannot. So once the
//! reading has reached the second-last beat before the barrier, the page
//! moves on to where it resumes — back for a repeat, ahead for a volta
//! skipped or a coda — if that is not on the page already.
//!
//! It follows *events*, not a tempo estimate: everything here is score
//! geometry (where the next note is engraved) and time (how long the turn
//! takes). Qt-free apart from the timers; the viewport is reached through a
//! Canvas interface the owner implements (mirroring HighlightFader /
//! KineticScroller).
class ScoreFollower
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
    //! The user's chosen zoom — the most zoomed-in the auto-zoom will ever go.
    virtual double defaultScaling() const = 0;
    //! Smallest zoom the auto zoom-out may reach.
    virtual double minScaling() const = 0;
    //! The logical x currently resting on the anchor (see anchorFrac) — the
    //! inverse of centerOn(), read back after someone else moved the view.
    virtual double anchorX() const = 0;
    //! Place logical x \p logicalX at the anchor at \p scaling, keeping the
    //! system vertically centered, and request a repaint.
    virtual void centerOn(double logicalX, double scaling) = 0;
  };

  //! Where the anchor rests horizontally in the view: where a page turn puts
  //! the next event to play, and where centerOn() places whatever x it is
  //! given. Well left of center so that the music read ahead of it — the rest
  //! of the width, which it then crosses before the next turn — is as much of
  //! the page as possible.
  static constexpr double anchorFrac = 1.0 / 10.0;

  //! The barline past which the reading does not carry on from the focus (see
  //! the class comment), and where it resumes.
  struct Barrier
  {
    //! Engraved x of the barline: the right edge of the last measure read
    //! before the jump (or of the score).
    double x = 0.0;
    //! Engraved x where the reading resumes after the barrier: behind it for
    //! a repeat — the start of the repeated section, which the last turn
    //! keeps on the page when it can — or ahead of it for a volta skipped or
    //! a coda. Unset for the final barline.
    std::optional<double> resumeX;
    //! Whether the focus has reached the second-last beat before the barrier:
    //! the reading is as good as through it, and the page moves on to
    //! resumeX (see the class comment).
    bool imminent = false;
  };

  explicit ScoreFollower(Canvas &canvas);

  ScoreFollower(const ScoreFollower &) = delete;
  ScoreFollower &operator=(const ScoreFollower &) = delete;

  //! Feed one transition batch.
  //! \p soundingX maps each hand (staff) that is *sounding* this batch to the
  //! engraved x of its onset — where that hand has got to on the page.
  //! \p leadingAny / \p trailingAny are the rightmost / leftmost onset x that
  //! is sounding *or* upcoming, used once to frame the start.
  //! \p nextX is what the reader needs to see next — the most imminent
  //! upcoming onset, or the leading sounding one when this batch pre-lights
  //! nothing — and is what the page keeps in view.
  //! \p barrier is the next barline past which the reading does not carry on
  //! from \p nextX — a repeat's end on the pass that jumps back, the point a
  //! volta or a jump leaves from, or the final barline — and, for a repeat,
  //! where it resumes (see Barrier). The last turn before it frames it rather
  //! than the focus (see the class comment).
  void onEvents(const std::map<int /*staff*/, double /*onsetX*/> &soundingX,
                std::optional<double> leadingAny,
                std::optional<double> trailingAny, std::optional<double> nextX,
                std::optional<Barrier> barrier);

  //! Advance the follow for one rendered frame. The owner calls this from the
  //! window's per-frame hook, so the motion is sampled in step with the
  //! display: a free-running timer at a nominal 60 Hz instead beats against
  //! the refresh (16 vs 16.67 ms), which shows up as a frame repeated and the
  //! next one double-stepped a couple of times a second. No-op unless a page
  //! is being followed.
  void frameTick();

  //! The user took manual control (a click, a swipe, a drag, a zoom): stop
  //! following until they play again, which re-frames.
  void suspend();

  //! The view moved under someone else's control (a drag, a swipe, a zoom, a
  //! relayout): pick the page up where it now is, so that the next framing
  //! glides from there rather than from where we last left it.
  void viewMoved();

  //! The position jumped (a rewind, a click on a note, a repeat): forget the
  //! hands and the framing, but keep the page where it is, so that the next
  //! events re-frame by gliding there from here rather than cutting.
  void jump();

  //! Forget everything (a new score): the next onsets re-frame with a cut.
  void reset();

private:
  void tick();

  //! One-shot framing at the start of a take: put \p leadingX on the anchor,
  //! zooming out if \p trailingX would not fit in the view with it.
  void frame(double leadingX, double trailingX);

  //! Whether resting \p focusX on the anchor at \p scaling would bring the
  //! barrier into view — which makes that turn the last one before it, and
  //! the barrier framing (below) what it does instead.
  bool barrierInView(double focusX, double scaling) const;
  //! The barrier framing's anchor at \p scaling (see the class comment) —
  //! short of pushing \p focusX off the left edge.
  double barrierAnchorX(double scaling, double focusX) const;

  Canvas &_canvas;
  QElapsedTimer _clock; // wall clock for event timestamps (ms)
  QTimer _timer;        // fallback driver when no frames are being rendered
  bool _framed = false;
  //! User took manual control; ignore events until reset.
  bool _suspended = false;
  double _scaling = 0.0;  // current (eased) zoom; 0 = unset
  qint64 _lastTickMs = 0; // for easing dt
  //! When the frame hook last drove a tick, so the timer knows whether frames
  //! are flowing (then it stays out of the way) or have stopped (then it
  //! takes over).
  qint64 _lastFrameTickMs = 0;

  //! The page scroll: the logical x resting on the anchor (_pageX), the one
  //! the current page turn is heading for (_pageTargetX — equal to _pageX at
  //! rest), the intermediate between them that makes the turn an S-curve
  //! rather than a jerk out of the gate (_pageEaseX — see tauPageMs), and the
  //! next event to play, which triggers the turns.
  double _pageX = std::numeric_limits<double>::quiet_NaN();
  double _pageEaseX = std::numeric_limits<double>::quiet_NaN();
  double _pageTargetX = std::numeric_limits<double>::quiet_NaN();
  //! The time constant of the glide in progress: a page turn's or, shorter, a
  //! relocation's.
  double _pageTauMs = 500.0;
  std::optional<double> _focusX;
  //! The barline the reading will not carry on through (see onEvents).
  std::optional<Barrier> _barrier;

  //! Where each hand last struck, and when. The auto-zoom uses it to keep
  //! hands that are playing at once on the same page; the timestamp is what
  //! lets a hand that has stopped playing drop out of that reckoning.
  struct Hand
  {
    double x = 0.0;
    qint64 lastOnsetMs = 0;
  };
  std::map<int /*staff*/, Hand> _hands;
};
} // namespace dgk
