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
//! then brought back to anchorFrac over an ease whose pace follows the
//! performer's own — speedFactor times it, so the page pulls ahead and
//! rests. Between turns nothing moves at all.
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
//! back finds it there and the page does not move at all. This last turn is
//! also triggered earlier than an ordinary one (barrierTriggerFrac): the
//! sooner it lands, the longer what it frames stands in view before the
//! jump.
//!
//! The jump itself can be anticipated (setAnticipateJumps — opt-in, off by
//! default). The last notes before it the performer can hold in their
//! fingers; the first notes after it they cannot. So the page moves on to
//! where the reading resumes — back for a repeat, ahead for a volta skipped
//! or a coda — if that is not on the page already, timed by the performer's
//! own tempo so that the glide is over jumpLeadMs before the first note
//! after the jump is due. Never, though, before the framing stands: the page
//! at rest, the barrier at least barrierMarginPx inside the right edge
//! (barrierFramed()) — the last notes before the barrier are what the
//! framing brings into view, and moving on mid-turn would take them off the
//! page unseen. A lead that falls due first starts the framing turn right
//! away (trigger or no trigger) and hurries it; the move follows it, late
//! but complete. Off, the page follows only the actual jump, relocating
//! after it happens.
//!
//! It follows *events*: everything here is score geometry (where the next
//! note is engraved) and time (when it was played) — the reading's pace is
//! measured from those (see _speedX); only the jump anticipation asks the
//! owner for a tempo-based estimate (resumeExpectedInMs). The zoom is the
//! user's and is never touched: the owner decides which event the page keeps in
//! view (see ReadingFocus) and the follower only scrolls. Qt-free apart from
//! the timers; the viewport is reached through a Canvas interface the owner
//! implements (mirroring HighlightFader / KineticScroller).
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
    //! The current zoom (physical px per logical unit) — the user's; the
    //! follower only reads it.
    virtual double viewScaling() const = 0;
    //! The logical x currently resting on the anchor (see anchorFrac) — the
    //! inverse of centerOn(), read back after someone else moved the view.
    virtual double anchorX() const = 0;
    //! Place logical x \p logicalX at the anchor, keeping the system
    //! vertically centered, and request a repaint.
    virtual void centerOn(double logicalX) = 0;
    //! When the first note after the jump ahead of the focus is expected to
    //! be played, in ms from now, at the performer's live tempo — or nothing
    //! without a live estimate. Asked every tick, and once more at the moment
    //! it names (see the class comment on anticipation).
    virtual std::optional<double> resumeExpectedInMs() const = 0;
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
    //! The unrolled (playback) tick the barrier stands at: tells one pass of
    //! a repeated barline from the next, which is the same x.
    int utick = 0;
    //! Engraved x where the reading resumes after the barrier: behind it for
    //! a repeat — the start of the repeated section, which the last turn
    //! keeps on the page when it can — or ahead of it for a volta skipped or
    //! a coda. Unset for the final barline.
    std::optional<double> resumeX;
  };

  explicit ScoreFollower(Canvas &canvas);

  ScoreFollower(const ScoreFollower &) = delete;
  ScoreFollower &operator=(const ScoreFollower &) = delete;

  //! Whether to anticipate jumps (see the class comment). A setting, not
  //! state: it survives reset(). Off by default.
  void setAnticipateJumps(bool anticipate);

  //! Feed one transition batch.
  //! \p struck: whether a note was struck this batch (as opposed to released,
  //! or merely pre-lit) — what resumes a suspended follow.
  //! \p startX is where the performer is on the page — the leading hand's
  //! last onset — used once to frame the start of a take.
  //! \p focusX is what the reader needs to see next (the owner's ReadingFocus)
  //! and is what the page keeps in view.
  //! \p barrier is the next barline past which the reading does not carry on
  //! from \p focusX — a repeat's end on the pass that jumps back, the point a
  //! volta or a jump leaves from, or the final barline — and, for a repeat,
  //! where it resumes (see Barrier). The last turn before it frames it rather
  //! than the focus (see the class comment).
  void onEvents(bool struck, std::optional<double> startX,
                std::optional<double> focusX, std::optional<Barrier> barrier);

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

  //! One-shot framing at the start of a take: put \p startX on the anchor.
  void frame(double startX);

  //! Whether resting \p focusX on the anchor would bring the barrier into
  //! view — which makes that turn the last one before it, and the barrier
  //! framing (below) what it does instead.
  bool barrierInView(double focusX) const;
  //! The barrier framing's anchor (see the class comment) — short of pushing
  //! \p focusX off the left edge.
  double barrierAnchorX(double focusX) const;
  //! Whether the page stands at rest with the barrier at least
  //! barrierMarginPx inside the right edge — what must hold before the
  //! anticipated move to the resume point may begin (see the class comment).
  bool barrierFramed() const;
  //! The τ that makes a turn over \p distance glide at speedFactor times the
  //! reading's measured pace (see _speedX), clamped to sanity.
  double turnTauMs(double distance) const;

  Canvas &_canvas;
  QElapsedTimer _clock; // wall clock for event timestamps (ms)
  QTimer _timer;        // fallback driver when no frames are being rendered
  bool _framed = false;
  //! User took manual control; ignore events until reset.
  bool _suspended = false;
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
  //! The time constant of the glide in progress: a turn's (tempo-
  //! proportional — see turnTauMs) or a relocation's.
  double _pageTauMs = 500.0;
  std::optional<double> _focusX;
  //! The barline the reading will not carry on through (see onEvents).
  std::optional<Barrier> _barrier;
  //! See setAnticipateJumps.
  bool _anticipateJumps = false;
  //! Whether the anticipated move to the resume point is under way — latched
  //! (no sooner than the barrier framing settles — see the class comment)
  //! until the barrier changes: the tempo estimate wavers, and a page that
  //! has moved on does not swing back.
  bool _jumping = false;
  //! Idle, the follower does not tick; but a jump ahead may fall due with no
  //! event to wake up to (a note held into it). This one-shot fires when the
  //! jump would be due at the estimate last seen, to look again.
  QTimer _jumpWake;

  //! The reading's pace across the page (logical x per ms), measured from the
  //! focus's own movement and smoothed over speedSmoothingMs of playing: what
  //! the turns' pace follows (see speedFactor / turnTauMs). Zero until two
  //! onsets apart in x have been seen.
  double _speedX = 0.0;
  //! The last focus sample the pace was measured against (see onEvents).
  double _speedSampleX = std::numeric_limits<double>::quiet_NaN();
  qint64 _speedSampleMs = 0;
};
} // namespace dgk
