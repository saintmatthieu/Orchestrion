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
#include "ScoreFollower.h"

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
// The barrier framing never pushes the focus nearer than this to the left
// edge (physical px).
constexpr double edgeMarginPx = 48.0;

// How far right the next event to play may drift before the page turns. It is
// then brought back to anchorFrac, so the page advances by the difference —
// most of a screen — at a time, and stands still in between.
constexpr double pageTriggerFrac = 4.0 / 5.0;

// The barrier framing's margins (physical px; see the class comment): the last
// turn before a barrier brings it at least this far inside the right edge, and
// lets at most this much of what precedes the repeat's start show at the left.
constexpr double barrierMarginPx = 100.0;

// The page turn is not a teleport: it glides over this time constant (per
// pole — the turn is two of them in series, so it settles in ~5 τ), short
// enough that the event is in place well before it is due.
constexpr double tauPageMs = 500.0;
// A relocation — a rewind, a repeat, a jump to elsewhere in the score — is
// not a page turn: there is no music to follow across the gap, so it must be
// quick, and a cut is disorienting. It glides with the same critically damped
// ease, over this fixed duration whatever the distance (the ease's speed
// scales with the distance, its duration does not): the two-pole step is
// within 4 % of its target at 5 τ, so τ is a fifth of it.
constexpr double relocateDurationMs = 750.0;
constexpr double tauRelocateMs = relocateDurationMs / 5.0;

// The anticipated move to where the reading resumes after a jump (see the
// class comment) is a relocation, and is to be over this long before the first
// note after the jump is due — so it starts relocateDurationMs earlier still.
constexpr double jumpLeadMs = 1000.0;

// Longest frame the easings will act on (ms). See the use site: it turns a
// dropped frame into a slightly late glide instead of a visible lurch.
constexpr double maxTickMs = 40.0;

// No frame-driven tick for this long means the window has stopped rendering
// (hidden, occluded, or simply nothing moving): the timer then drives.
constexpr qint64 frameStallMs = 40;

// The glide is settled once it is within this many physical pixels of its
// target, and the canvas is only placed again once the page has moved by at
// least this many since it was last placed (see the use site).
constexpr double settlePx = 0.25;
constexpr double placeStepPx = 0.5;
} // namespace

ScoreFollower::ScoreFollower(Canvas &canvas) : _canvas{canvas}
{
  _clock.start();
  _timer.setInterval(16); // ~60 fps
  _timer.callOnTimeout(
      [this]
      {
        // The frame hook (frameTick) drives the follow whenever the window is
        // rendering; ticking here as well would sample the motion at times
        // unrelated to the refresh, which is the judder frameTick avoids.
        if (_clock.elapsed() - _lastFrameTickMs > frameStallMs)
          tick();
      });
  _jumpWake.setSingleShot(true);
  _jumpWake.callOnTimeout(
      [this]
      {
        if (!_timer.isActive())
          _timer.start();
      });
}

void ScoreFollower::onEvents(bool struck, std::optional<double> startX,
                             std::optional<double> focusX,
                             std::optional<Barrier> barrier)
{
  if (_suspended)
  {
    // A manual click/swipe/zoom suspended us; resume only when a note is
    // actually played again, and start fresh so we re-frame where the
    // performer now is — gliding there from wherever the view is (see
    // viewMoved()).
    if (!struck)
      return;
    jump();
  }

  // What the page keeps in view (see pageTriggerFrac), and the barrier ahead
  // of it (see barrierMarginPx). Held until the next batch: between events
  // there is nothing new to react to.
  const bool focusMoved = focusX && (!_focusX || *focusX != *_focusX);
  if (focusX)
  {
    _focusX = focusX;
    // A different barrier — the reading is through the old one — releases
    // the anticipated move (see _jumping).
    const bool sameBarrier =
        barrier && _barrier && barrier->utick == _barrier->utick;
    if (!sameBarrier)
      _jumping = false;
    _barrier = barrier;
  }

  // One-shot framing once we have a laid-out viewport and a position.
  if (!_framed && startX && _canvas.viewWidth() > 1.0)
    frame(*startX);

  // A batch that only pre-lights the next chord still moves the focus, and
  // may be the one that puts it past the trigger: wake up for that too, not
  // just for a struck note.
  if ((focusMoved || struck) && !_timer.isActive())
    _timer.start();
}

void ScoreFollower::frame(double startX)
{
  // Never past the barrier framing (see the class comment): a take that
  // starts inside a short repeated section opens on the section's start, not
  // on what follows the repeat.
  const double targetX = std::min(startX, barrierAnchorX(startX));
  if (std::isfinite(_pageX))
  {
    // The page is still where the previous take (or the position before a
    // jump) left it: glide from there rather than cut.
    _pageTargetX = targetX;
    _pageTauMs = tauRelocateMs;
    place(_pageX);
  }
  else
  {
    _pageX = _pageEaseX = _pageTargetX = targetX;
    place(targetX);
  }
  _framed = true;
}

void ScoreFollower::place(double pageX)
{
  _canvas.centerOn(pageX);
  _placedX = pageX;
}

bool ScoreFollower::barrierInView(double focusX) const
{
  const double scaling = _canvas.viewScaling();
  if (!_barrier || scaling <= 0.0)
    return false;
  // With the focus on the anchor, the view reaches (1 - anchorFrac) of the
  // width beyond it.
  const double logicalWidth = _canvas.viewWidth() / scaling;
  return _barrier->x < focusX + (1.0 - anchorFrac) * logicalWidth;
}

double ScoreFollower::barrierAnchorX(double focusX) const
{
  const double scaling = _canvas.viewScaling();
  if (!_barrier || scaling <= 0.0)
    return std::numeric_limits<double>::infinity();
  const double logicalWidth = _canvas.viewWidth() / scaling;
  const double margin = barrierMarginPx / scaling;
  // The right edge: at least the margin beyond the barrier, and — for a
  // repeat — at least the width less the margin beyond the section's start,
  // so that no more than the margin of what precedes the start shows. Both
  // are floors; the higher decides.
  double rightX = _barrier->x + margin;
  if (_barrier->resumeX && *_barrier->resumeX < _barrier->x)
    rightX = std::max(rightX, *_barrier->resumeX - margin + logicalWidth);
  // The anchor is (1 - anchorFrac) of the width short of the right edge.
  const double x = rightX - (1.0 - anchorFrac) * logicalWidth;
  // A section start on the page puts the focus (past it) comfortably in view.
  // Without one on the page — the final barline, a long section — the barrier
  // may lie nearer the focus than the margin supposes; never let the framing
  // push the focus off the left edge.
  return std::min(x,
                  focusX + anchorFrac * logicalWidth - edgeMarginPx / scaling);
}

void ScoreFollower::frameTick()
{
  if (!_timer.isActive())
    return; // not following: nothing to advance
  _lastFrameTickMs = _clock.elapsed();
  tick();
}

void ScoreFollower::tick()
{
  const double now = static_cast<double>(_clock.elapsed());

  // The easing below is time-based, which is what keeps it independent of
  // the frame rate — but it also means a *lost* frame is compensated in one
  // step: after a 300 ms hitch (an audio hiccup, a relayout, a repaint that
  // overran) a Δ that should have taken 20 frames is applied in one, and the
  // score visibly lurches. Cap the step at one slow frame's worth and let the
  // remainder catch up over the following frames: the glide finishes a hair
  // later, but never jumps. (Idling zeroes _lastTickMs, so waking after a
  // pause starts from a nominal frame rather than from the whole pause.)
  const double dtMs =
      std::min(_lastTickMs > 0 ? now - _lastTickMs : 16.0, maxTickMs);
  _lastTickMs = static_cast<qint64>(now);

  // Page scroll. The event the reader needs to see next is the next one to
  // play; while it is comfortably inside the view the page does not move at
  // all, and when it would pass the trigger mark the page turns so that the
  // event lands on the anchor.
  const double scaling = _canvas.viewScaling();
  const double logicalWidth =
      scaling > 0.0 ? _canvas.viewWidth() / scaling : 0.0;
  if (!_focusX)
    return; // nothing played yet: nowhere to be
  // The jump ahead is anticipated (see the class comment): from the moment a
  // glide started now would be over jumpLeadMs before the first note after
  // it is due, the reading is as good as through the barrier. What is left
  // before it the performer has under their fingers; what they need to see is
  // where it resumes — so that is the focus from here on. Plain, without the
  // barrier framing, which concerns the section being left: the resume point
  // lands on the anchor if it is off the page (or past the trigger), and
  // nothing moves if it is not.
  std::optional<double> resumeInMs;
  if (!_jumping && _barrier && _barrier->resumeX)
  {
    resumeInMs = _canvas.resumeExpectedInMs();
    if (resumeInMs && *resumeInMs <= jumpLeadMs + relocateDurationMs)
      _jumping = true;
  }
  const bool jumping = _jumping && _barrier && _barrier->resumeX;
  const double focusX = jumping ? *_barrier->resumeX : *_focusX;
  if (!std::isfinite(_pageTargetX) || logicalWidth <= 0.0)
    _pageX = _pageEaseX = _pageTargetX = focusX;
  else
  {
    // Where the focus sits in the view, given that _pageTargetX rests on the
    // anchor. Past the trigger mark: a page turn. Off the left edge (a
    // repeat, a rewind) or off the right one (a jump ahead): a relocation,
    // which glides too, but over a fixed short duration (see
    // relocateDurationMs).
    const double frac = anchorFrac + (focusX - _pageTargetX) / logicalWidth;
    // Where either would put the anchor: on the focus — unless that would
    // bring the barrier into view, which makes this the last turn before it,
    // and the barrier framing (see the class comment) what it does instead.
    const double turnX =
        !jumping && barrierInView(focusX) ? barrierAnchorX(focusX) : focusX;
    if (frac < 0.0 || frac > 1.0)
    {
      _pageTargetX = turnX;
      _pageTauMs = tauRelocateMs;
    }
    else if (frac > pageTriggerFrac && turnX > _pageTargetX)
    {
      // (A turn only ever advances: once the framing is in place, the focus
      // drifting on past the trigger asks for the same framing again — or,
      // when the page already shows more than it, for a step back — and the
      // page stands still, the focus in view all the way to the barrier.)
      // A jump is a relocation even when its target is on the page: quick,
      // and over when the lead says.
      _pageTargetX = turnX;
      _pageTauMs = jumping ? tauRelocateMs : tauPageMs;
    }
  }

  // Two identical lags in series, not one. A single pole leaves the target's
  // jump as a step in *velocity* — v(0) = Δ/τ is its maximum — which reads as
  // a jerk at the start of the turn. Cascading them gives the critically
  // damped Δ·(1 − (1 + t/τ)·e^(−t/τ)), whose velocity starts from rest, peaks
  // at t = τ and eases out again: an S-curve, at the cost of settling in
  // ~5 τ instead of ~3 τ.
  // The ease only ever approaches its target; it is called settled within a
  // fraction of a physical pixel, where nothing it could still do would show.
  const double settleTolerance = settlePx / scaling;
  if (std::abs(_pageTargetX - _pageX) > settleTolerance ||
      std::abs(_pageTargetX - _pageEaseX) > settleTolerance)
  {
    const double k = 1.0 - std::exp(-dtMs / _pageTauMs);
    _pageEaseX += (_pageTargetX - _pageEaseX) * k;
    _pageX += (_pageEaseX - _pageX) * k;
  }
  else
    _pageX = _pageEaseX = _pageTargetX;

  // Placing the canvas repaints the whole score (see Canvas::centerOn), so
  // it is done only for a move that shows — the ease's long tail creeps by
  // less than a pixel a frame — and once more when the page comes to rest.
  const bool settled = _pageX == _pageTargetX;
  if (settled || !std::isfinite(_placedX) ||
      std::abs(_pageX - _placedX) * scaling >= placeStepPx)
    place(_pageX);

  // Nothing moves between page turns, so once the turn has settled there is
  // nothing left to animate: idle until the next event restarts the timer —
  // or, with a jump ahead, until it would be due at the estimate just seen: a
  // note held into the jump brings no event to wake up to. (Looked at afresh
  // then: a performer who has frozen meanwhile has not got any nearer to it.)
  if (settled)
  {
    _timer.stop();
    _lastTickMs = 0;
    if (resumeInMs && !_jumping)
      _jumpWake.start(static_cast<int>(
          std::max(16.0, *resumeInMs - jumpLeadMs - relocateDurationMs)));
  }
}

void ScoreFollower::suspend()
{
  _suspended = true;
  _timer.stop();
  _jumpWake.stop();
}

void ScoreFollower::viewMoved()
{
  // Where we left the page is no longer where it is: pick it up from the view
  // itself, so the next framing glides from the right spot.
  _pageX = _pageEaseX = _pageTargetX = _canvas.anchorX();
  _placedX = std::numeric_limits<double>::quiet_NaN();
}

void ScoreFollower::jump()
{
  // Keep the page where it is — the glide to the new location starts there —
  // and forget the rest.
  const double pageX = _pageX;
  reset();
  _pageX = _pageEaseX = pageX;
}

void ScoreFollower::reset()
{
  _timer.stop();
  _jumpWake.stop();
  _framed = false;
  _suspended = false;
  _lastTickMs = 0;
  _lastFrameTickMs = 0;
  _pageX = std::numeric_limits<double>::quiet_NaN();
  _pageEaseX = std::numeric_limits<double>::quiet_NaN();
  _pageTargetX = std::numeric_limits<double>::quiet_NaN();
  _placedX = std::numeric_limits<double>::quiet_NaN();
  _pageTauMs = tauPageMs;
  _focusX.reset();
  _barrier.reset();
  _jumping = false;
}
} // namespace dgk
