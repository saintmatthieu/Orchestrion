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
#include "ComputerKeyboard.h"
#ifdef Q_OS_LINUX
#include "RawKeyboard.h"
#endif
#include <QApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QLocale>
#include <QQuickItem>
#include <global/settings.h>
#include <log.h>

namespace dgk
{
namespace
{
/**
 * Whether the keys are meant for something other than playing: a text field
 * (such as the score shelf's search box), or a panel that says so with a
 * `suppressesGestureKeys` property (such as the score shelf, whose keys
 * choose a score).
 */
bool keysAreForSomethingElse()
{
  const auto item = qobject_cast<const QQuickItem *>(qApp->focusObject());
  if (item && item->flags().testFlag(QQuickItem::ItemAcceptsInputMethod))
    return true;
  for (const QQuickItem *ancestor = item; ancestor;
       ancestor = ancestor->parentItem())
    if (ancestor->property("suppressesGestureKeys").toBool())
      return true;
  return false;
}
} // namespace

ComputerKeyboardImpl::ComputerKeyboardImpl(ComputerKeyboard &owner)
    : QObject{nullptr}, m_owner(owner)
{
}

void ComputerKeyboardImpl::init()
{
  qApp->installEventFilter(this);
  if (!m_owner.raw())
    return;
  connect(qApp, &QGuiApplication::focusObjectChanged, this,
          [this] { updatePlayable(); });
  connect(qApp, &QGuiApplication::applicationStateChanged, this,
          [this] { updatePlayable(); });
  m_playableTimer.callOnTimeout(this, [this] { updatePlayable(); });
  m_playableTimer.start(100);
  updatePlayable();
}

void ComputerKeyboardImpl::updatePlayable()
{
#ifdef Q_OS_LINUX
  m_owner.raw()->SetPlayable(QGuiApplication::applicationState() ==
                                 Qt::ApplicationActive &&
                             !keysAreForSomethingElse());
#endif
}

bool ComputerKeyboardImpl::eventFilter(QObject *watched, QEvent *event)
{
  const auto modifiers = QGuiApplication::queryKeyboardModifiers();
  const bool isKeyModifierPressed =
      modifiers & Qt::KeyboardModifier::ControlModifier ||
      modifiers & Qt::KeyboardModifier::AltModifier ||
      modifiers & Qt::KeyboardModifier::ShiftModifier;
  // Those of the keyboard itself are read by the raw keyboard, if there is
  // one: only the synthesized ones (a demo's) are left to this.
  const bool handledElsewhere = m_owner.raw() && event->spontaneous();
  if (!isKeyModifierPressed && !handledElsewhere)
  {
    // Only the presses: a key held when the focus moved must still be
    // released.
    if (event->type() == QEvent::KeyPress && !keysAreForSomethingElse())
    {
      auto keyEvent = static_cast<const QKeyEvent *>(event);
      if (keyEvent->isAutoRepeat())
        return false;
      m_owner.onKey(keyEvent->key(), true);
    }
    else if (event->type() == QEvent::KeyRelease)
    {
      auto keyEvent = static_cast<const QKeyEvent *>(event);
      if (keyEvent->isAutoRepeat())
        return false;
      m_owner.onKey(keyEvent->key(), false);
    }
  }
  return QObject::eventFilter(watched, event);
}

ComputerKeyboard::ComputerKeyboard()
    : m_impl{std::make_unique<ComputerKeyboardImpl>(*this)}
{
}

ComputerKeyboard::~ComputerKeyboard() = default;

void ComputerKeyboard::preInit()
{
#ifdef Q_OS_LINUX
  // Not rendering offline, on the offscreen platform: there, all happens on
  // the main thread, whose clock is virtual.
  if (QGuiApplication::platformName() == QLatin1String("xcb"))
  {
    m_raw = RawKeyboard::Create([this](char key, bool pressed)
                                { onKey(key, pressed); });
    if (m_raw)
      LOGI() << "Keys read from X11's raw events, off the GUI thread";
    else
      LOGW() << "No XInput 2.1: keys read from Qt's events";
  }
#endif
  m_impl->init();
}

void ComputerKeyboard::onKey(char letter, bool pressed)
{
  {
    std::scoped_lock lock{m_receiverMutex};
    if (m_receiver)
      m_receiver(letter, pressed);
  }
  (pressed ? m_keyPressed : m_keyReleased).send(letter);
}

void ComputerKeyboard::setReceiver(Receiver receiver)
{
  std::scoped_lock lock{m_receiverMutex};
  m_receiver = std::move(receiver);
}

muse::async::Channel<char> ComputerKeyboard::keyPressed() const
{
  return m_keyPressed;
}

muse::async::Channel<char> ComputerKeyboard::keyReleased() const
{
  return m_keyReleased;
}
} // namespace dgk