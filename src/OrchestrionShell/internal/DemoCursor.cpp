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
#include <algorithm>
#include <utility>

namespace dgk
{
namespace
{
// A glide takes longer the farther it goes, as a hand's would, within bounds.
int glideMs(qreal distance)
{
  return static_cast<int>(std::clamp(300.0 + 0.6 * distance, 350.0, 1100.0));
}

// Pauses that let the eye follow: on arrival before pressing, the press
// itself, and after releasing.
constexpr int settleMs = 150;
constexpr int holdMs = 120;
constexpr int afterClickMs = 250;
} // namespace

DemoCursor::DemoCursor(QObject *parent) : QObject(parent)
{
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
  QQuickItem *const item = findItem(controlPath, ret);
  if (!item)
  {
    done(ret);
    return;
  }
  const QPointF target = item->mapToScene(item->boundingRect().center());
  glideTo(target,
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
  QQuickItem *const item = findItem(controlPath, ret);
  if (!item)
  {
    done(ret);
    return;
  }
  glideTo(item->mapToScene(item->boundingRect().center()),
          [done] { done(muse::make_ok()); });
}

void DemoCursor::hide()
{
  m_glide.stop();
  m_afterGlide = nullptr;
  if (!m_shown)
    return;
  m_shown = false;
  emit shownChanged();
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
