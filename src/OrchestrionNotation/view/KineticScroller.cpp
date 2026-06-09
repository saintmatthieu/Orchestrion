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
#include "KineticScroller.h"
#include <QWheelEvent>
#include <cmath>

namespace dgk
{
namespace
{
constexpr qreal kMinVelocity = 80.;   // px/s; below this a flick doesn't glide
constexpr qreal kMaxVelocity = 8000.; // px/s; cap an over-enthusiastic flick
constexpr qreal kTau = 0.35;          // s; deceleration time constant ("feel")
constexpr qint64 kWindowMs = 80;      // recent-motion window for the estimate
constexpr int kFlingIntervalMs = 16;  // ~60 Hz glide ticks
constexpr int kIdleIntervalMs = 60;   // gesture-end timeout when no ScrollEnd
} // namespace

KineticScroller::KineticScroller(MoveByFn moveBy) : m_moveBy(std::move(moveBy))
{
  m_flingTimer.setInterval(kFlingIntervalMs);
  m_flingTimer.callOnTimeout([this] { onFlingTick(); });

  // When the platform doesn't report a ScrollEnd phase (e.g. X11), a swipe is
  // just a burst of discrete events; we treat a short gap as the end of it.
  m_idleTimer.setSingleShot(true);
  m_idleTimer.setInterval(kIdleIntervalMs);
  m_idleTimer.callOnTimeout([this] { startFling(); });
}

bool KineticScroller::handleWheelEvent(const QWheelEvent &event, qreal viewWidth)
{
  if (!m_clock.isValid())
    m_clock.start();
  const qint64 now = m_clock.elapsed();

  const Qt::ScrollPhase phase = event.phase();
  if (phase == Qt::ScrollBegin)
  {
    // A fresh gesture interrupts an ongoing glide and starts from rest.
    m_flingTimer.stop();
    m_samples.clear();
  }

  QPoint pixels = event.pixelDelta();
  const QPoint angle = event.angleDelta();

#ifdef Q_OS_LINUX
  // pixelDelta is unreliable on X11; only trust it under Wayland (same caveat
  // as MuseScore's NotationViewInputController::wheelEvent).
  if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
    pixels = QPoint{};
#endif

  qreal dx = 0.;
  if (pixels.x() != 0)
    dx = pixels.x();
  else if (angle.x() != 0)
    dx = angle.x() * qMax(2.0, viewWidth / 10.0) /
         QWheelEvent::DefaultDeltasPerStep;

  bool consumed = false;
  if (!qFuzzyIsNull(dx))
  {
    // Genuine motion interrupts an ongoing glide (covers X11, where there is no
    // ScrollBegin). A motionless end-of-gesture event must NOT stop it, or a
    // duplicate ScrollEnd would tear down the glide we just started.
    m_flingTimer.stop();
    m_moveBy(dx);
    m_samples.emplace_back(dx, now); // for the release-velocity estimate
    consumed = true;
  }

  // Decide when the gesture has ended and the glide should take over.
  if (phase == Qt::ScrollEnd)
    startFling();
  else if (phase == Qt::NoScrollPhase)
    // No begin/end notifications (e.g. X11): infer the end from a short idle.
    m_idleTimer.start();
  else
    // ScrollBegin/ScrollUpdate: a ScrollEnd is coming, so wait for it.
    m_idleTimer.stop();

  return consumed;
}

void KineticScroller::beginDrag()
{
  if (!m_clock.isValid())
    m_clock.start();
  m_flingTimer.stop(); // a fresh gesture interrupts any glide
  m_idleTimer.stop();
  m_samples.clear();
}

void KineticScroller::addDragSample(qreal physicalDx)
{
  // Sample only: during a drag the owner already moves the view, so moving here
  // too would double the motion. We just measure it for the release velocity.
  if (qFuzzyIsNull(physicalDx))
    return;
  if (!m_clock.isValid())
    m_clock.start();
  m_samples.emplace_back(physicalDx, m_clock.elapsed());
}

void KineticScroller::endDrag() { startFling(); }

qreal KineticScroller::sampledVelocity()
{
  // Average velocity (physical px/s) over motion in the window ending at *now*
  // (the release moment). Pruning against now means a trailing gap — slowing
  // down or holding still before lifting — leaves the window empty, so the
  // throw decays to zero. A lone stray event near release (some trackpads emit
  // one) isn't sustained motion either, so at least two samples are required.
  const qint64 now = m_clock.isValid() ? m_clock.elapsed() : 0;
  while (!m_samples.empty() && now - m_samples.front().second > kWindowMs)
    m_samples.pop_front();

  if (m_samples.size() < 2)
    return 0.;

  qreal netDx = 0.;
  for (const auto &sample : m_samples)
    netDx += sample.first;
  const qint64 spanMs = qMax<qint64>(now - m_samples.front().second, 8);
  return netDx / (spanMs / 1000.0);
}

void KineticScroller::startFling()
{
  // Trackpads can deliver more than one ScrollEnd per gesture; ignore the
  // duplicates so they don't recompute (from now-empty samples) and cancel the
  // glide we just started.
  if (m_flingTimer.isActive())
    return;

  m_idleTimer.stop();
  m_velocity = sampledVelocity();
  m_samples.clear();

  if (qAbs(m_velocity) < kMinVelocity)
  {
    m_velocity = 0.;
    return;
  }
  m_velocity = qBound(-kMaxVelocity, m_velocity, kMaxVelocity);
  m_flingClock.start();
  m_flingTimer.start();
}

void KineticScroller::onFlingTick()
{
  const qreal dt = m_flingClock.restart() / 1000.0;
  if (dt <= 0.)
    return;

  const bool moved = m_moveBy(m_velocity * dt);

  // Exponential deceleration.
  m_velocity *= std::exp(-dt / kTau);

  if (!moved || qAbs(m_velocity) < kMinVelocity)
    stop();
}

void KineticScroller::stop()
{
  m_flingTimer.stop();
  m_idleTimer.stop();
  m_samples.clear();
  m_velocity = 0.;
}
} // namespace dgk
