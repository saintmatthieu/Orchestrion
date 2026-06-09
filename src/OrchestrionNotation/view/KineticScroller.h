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
#include <deque>
#include <functional>
#include <utility>

class QWheelEvent;

namespace dgk
{
//! Adds kinetic ("flick") horizontal scrolling to a view that can be panned in
//! physical pixels. The owner feeds it wheel events and supplies a callback
//! that performs the actual pan; the scroller takes care of the immediate
//! movement, the release-velocity estimate, and the decelerating glide.
//!
//! Everything here is in *physical* pixels; converting to the view's own
//! coordinates (e.g. dividing by a zoom factor) is the callback's job.
class KineticScroller
{
public:
  //! Pan the content by \p physicalDx physical pixels. Returns whether it
  //! actually moved: false means it is clamped at an edge, which stops a glide.
  using MoveByFn = std::function<bool(qreal physicalDx)>;

  explicit KineticScroller(MoveByFn moveBy);

  KineticScroller(const KineticScroller &) = delete;
  KineticScroller &operator=(const KineticScroller &) = delete;

  //! Feed one wheel event. \p viewWidth converts mouse-wheel notches
  //! (angleDelta) to a pixel delta. Returns true if the event carried
  //! horizontal motion (and was therefore consumed).
  bool handleWheelEvent(const QWheelEvent &event, qreal viewWidth);

  //! Abort any in-progress glide (e.g. on a click, score jump, notation reload).
  void stop();

private:
  void startFling();
  void onFlingTick();
  qreal sampledVelocity();

  MoveByFn m_moveBy;
  QTimer m_flingTimer;        // ~60 Hz tick that drives the glide
  QTimer m_idleTimer;         // infers gesture end when no ScrollEnd is delivered
  QElapsedTimer m_clock;      // free-running; timestamps wheel samples
  QElapsedTimer m_flingClock; // time between glide ticks
  // Recent (deltaX [physical px], timestamp [ms]) samples. The throw velocity
  // is estimated from *recent* motion only, so slowing down or holding still
  // before releasing (samples age out) produces no throw.
  std::deque<std::pair<qreal, qint64>> m_samples;
  qreal m_velocity = 0.; // physical px/s; sign matches the swipe direction
};
} // namespace dgk
