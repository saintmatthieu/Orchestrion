/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "DemoCursor.h"

#include <QCoreApplication>
#include <QLineF>
#include <QMouseEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>

namespace dgk
{
namespace
{
// A glide takes longer the farther it goes, as a hand's would, within bounds.
int glideMs(qreal distance)
{
  return static_cast<int>(std::clamp(500.0 + 1.2 * distance, 600.0, 2000.0));
}

// Pauses that let the eye follow: on arrival before pressing, the press
// itself, and after releasing.
constexpr int settleMs = 150;
constexpr int holdMs = 120;
constexpr int afterClickMs = 250;
} // namespace

DemoCursor::DemoCursor(QObject *parent) : QObject(parent)
{
  m_wheelClock.start();
  m_glide.setEasingCurve(QEasingCurve::InOutCubic);
  connect(&m_glide, &QVariantAnimation::valueChanged, this,
          [this](const QVariant &value)
          {
            setPosition(value.toPointF());
            sendMouse(QEvent::MouseMove, Qt::NoButton, Qt::NoButton);
          });
  connect(&m_glide, &QAbstractAnimation::finished, this,
          [this]
          {
            if (const auto then = std::exchange(m_afterGlide, nullptr))
              then();
          });
}

void DemoCursor::click(const QString &controlPath, Done done)
{
  muse::Ret ret;
  const auto target = targetPosition(controlPath, ret);
  if (!target)
  {
    done(ret);
    return;
  }
  glideTo(*target,
          [this, done]
          {
            QTimer::singleShot(settleMs, this,
                               [this, done]
                               {
                                 setPressed(true);
                                 sendMouse(QEvent::MouseButtonPress,
                                           Qt::LeftButton, Qt::LeftButton);
                                 QTimer::singleShot(
                                     holdMs, this,
                                     [this, done]
                                     {
                                       setPressed(false);
                                       sendMouse(QEvent::MouseButtonRelease,
                                                 Qt::LeftButton, Qt::NoButton);
                                       QTimer::singleShot(
                                           afterClickMs, this,
                                           [done] { done(muse::make_ok()); });
                                     });
                               });
          });
}

void DemoCursor::moveTo(const QString &controlPath, Done done)
{
  muse::Ret ret;
  const auto target = targetPosition(controlPath, ret);
  if (!target)
  {
    done(ret);
    return;
  }
  glideTo(*target, [done] { done(muse::make_ok()); });
}

void DemoCursor::scroll(int pixels)
{
  QWindow *const window = mainWindow()->qWindow();
  QWheelEvent event(m_position, window->mapToGlobal(m_position),
                    QPoint(0, pixels), QPoint(0, pixels), Qt::NoButton,
                    Qt::NoModifier, Qt::NoScrollPhase, false,
                    // As a touchpad's: Flickable then moves by the pixels, a
                    // mouse wheel's it would turn into a flick
                    Qt::MouseEventSynthesizedBySystem);
  // Flickable ignores what has no timestamp
  event.setTimestamp(static_cast<quint64>(m_wheelClock.elapsed()) + 1);
  QCoreApplication::sendEvent(window, &event);
}

void DemoCursor::waitUntilGone(const QString &path, Done done)
{
  const auto timer = std::make_shared<QTimer>();
  QObject::connect(timer.get(), &QTimer::timeout, this,
                   [this, timer, path, done]
                   {
                     if (findNamedItem(path))
                       return;
                     timer->stop();
                     done(muse::make_ok());
                   });
  timer->start(100);
}

void DemoCursor::hide()
{
  m_glide.stop();
  m_afterGlide = nullptr;
  if (!m_shown)
    return;
  m_shown = false;
  emit shownChanged();
  // Gone, as a pointer leaving the window: what it hovered (a tooltip, ...)
  // is let go of.
  QEvent leave(QEvent::Leave);
  QCoreApplication::sendEvent(mainWindow()->qWindow(), &leave);
}

QQuickItem *DemoCursor::findItem(const QString &controlPath,
                                 muse::Ret &ret) const
{
  const QStringList parts = controlPath.split('/');
  const muse::ui::INavigationControl *const control =
      parts.size() == 3
          ? navigationController()->findControl(parts[0].toStdString(),
                                                parts[1].toStdString(),
                                                parts[2].toStdString())
          : nullptr;
  QQuickItem *const item = control ? control->visualItem() : nullptr;
  if (!item)
  {
    ret = muse::make_ret(muse::Ret::Code::UnknownError,
                         "no control " + controlPath.toStdString());
    return nullptr;
  }
  if (item->window() != mainWindow()->qWindow())
  {
    ret =
        muse::make_ret(muse::Ret::Code::NotSupported,
                       controlPath.toStdString() + " isn't in the main window");
    return nullptr;
  }
  return item;
}

QQuickItem *DemoCursor::findNamedItem(const QString &path) const
{
  const auto window = qobject_cast<QQuickWindow *>(mainWindow()->qWindow());
  if (!window)
    return nullptr;
  const QString name = u"demo:" + path;
  const std::function<QQuickItem *(QQuickItem *)> find =
      [&](QQuickItem *item) -> QQuickItem *
  {
    if (!item->isVisible())
      return nullptr;
    if (item->objectName() == name)
      return item;
    for (QQuickItem *const child : item->childItems())
      if (QQuickItem *const found = find(child))
        return found;
    return nullptr;
  };
  return find(window->contentItem());
}

std::optional<QPointF> DemoCursor::targetPosition(const QString &path,
                                                  muse::Ret &ret) const
{
  if (path.startsWith(u"Score/"))
  {
    const QString name = path.mid(6);
    if (const auto position = scoreTargets()->position(name))
      return position;
    ret = muse::make_ret(muse::Ret::Code::UnknownError,
                         "nothing on the score is " + name.toStdString());
    return std::nullopt;
  }
  QQuickItem *item = findNamedItem(path);
  if (!item)
    item = findItem(path, ret);
  if (!item)
    return std::nullopt;
  // Not boundingRect(): a text field's follows its text
  return item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
}

void DemoCursor::glideTo(QPointF target, std::function<void()> then)
{
  if (!m_shown)
  {
    // Comes in from the middle of the window, which is where the eye is.
    const QWindow *const window = mainWindow()->qWindow();
    setPosition(QPointF(window->width() / 2.0, window->height() / 2.0));
    m_shown = true;
    emit shownChanged();
  }
  // A glide cut short by another never reaches its end, nor what was to
  // follow it.
  m_glide.stop();
  m_afterGlide = std::move(then);
  m_glide.setStartValue(m_position);
  m_glide.setEndValue(target);
  m_glide.setDuration(glideMs(QLineF(m_position, target).length()));
  m_glide.start();
}

void DemoCursor::sendMouse(QEvent::Type type, Qt::MouseButton button,
                           Qt::MouseButtons buttons)
{
  QWindow *const window = mainWindow()->qWindow();
  QMouseEvent event(type, m_position, m_position,
                    window->mapToGlobal(m_position), button, buttons,
                    Qt::NoModifier);
  QCoreApplication::sendEvent(window, &event);
}

bool DemoCursor::shown() const { return m_shown; }

QPointF DemoCursor::position() const { return m_position; }

bool DemoCursor::pressed() const { return m_pressed; }

void DemoCursor::setPosition(QPointF position)
{
  if (position == m_position)
    return;
  m_position = position;
  emit positionChanged();
}

void DemoCursor::setPressed(bool pressed)
{
  if (pressed == m_pressed)
    return;
  m_pressed = pressed;
  emit pressedChanged();
}
} // namespace dgk
