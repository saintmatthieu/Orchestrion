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
#if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
#define ORCHESTRION_RAW_KEYBOARD
#include "RawKeyboard.h"
#endif
#include <QApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QLocale>
#include <QQuickItem>
#include <QThread>
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
#ifdef ORCHESTRION_RAW_KEYBOARD
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
  if (!handledElsewhere)
  {
    // Only the presses are conditional: a key held when the focus moved, or
    // when a modifier went down, must still be released.
    if (event->type() == QEvent::KeyPress && !isKeyModifierPressed &&
        !keysAreForSomethingElse())
    {
      auto keyEvent = static_cast<const QKeyEvent *>(event);
      if (keyEvent->isAutoRepeat())
        return false;
      if (const quint32 scanCode = keyEvent->nativeScanCode())
        m_pressedKeys[scanCode] = keyEvent->key();
      m_owner.onKey(keyEvent->key(), true);
    }
    else if (event->type() == QEvent::KeyRelease)
    {
      auto keyEvent = static_cast<const QKeyEvent *>(event);
      if (keyEvent->isAutoRepeat())
        return false;
      // The key as it was pressed: with Shift down since, it reads otherwise.
      int key = keyEvent->key();
      if (const auto it = m_pressedKeys.find(keyEvent->nativeScanCode());
          it != m_pressedKeys.end())
      {
        key = it->second;
        m_pressedKeys.erase(it);
      }
      m_owner.onKey(key, false);
    }
  }
  return QObject::eventFilter(watched, event);
}

ComputerKeyboard::ComputerKeyboard()
    : m_impl{std::make_unique<ComputerKeyboardImpl>(*this)}
{
}

ComputerKeyboard::~ComputerKeyboard()
{
  // First: until its thread is joined, it calls onKey(), which uses the
  // members declared after it.
  m_raw.reset();
}

void ComputerKeyboard::preInit()
{
#ifdef ORCHESTRION_RAW_KEYBOARD
  // Not rendering offline, on the offscreen platform: there, all happens on
  // the main thread, whose clock is virtual. (macOS has no raw keyboard: one
  // would need the Input Monitoring permission.)
  const QString platform = QGuiApplication::platformName();
  if (platform == QLatin1String("xcb") || platform == QLatin1String("windows"))
  {
    m_raw = RawKeyboard::Create([this](char key, bool pressed)
                                { onKey(key, pressed); });
    if (m_raw)
      LOGI() << "Keys read from raw input events, off the GUI thread";
    else
      LOGW() << "No raw keyboard input: keys read from Qt's events";
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
  const auto send = [this, letter, pressed]
  { (pressed ? m_keyPressed : m_keyReleased).send(letter); };
  if (QThread::currentThread() == m_mainThreadContext.thread())
    send();
  else
    QMetaObject::invokeMethod(&m_mainThreadContext, send, Qt::QueuedConnection);
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