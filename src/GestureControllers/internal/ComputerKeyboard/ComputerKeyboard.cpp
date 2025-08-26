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
#include <QApplication>
#include <QInputMethod>
#include <QKeyEvent>
#include <QLocale>
#include <global/settings.h>

namespace dgk
{
ComputerKeyboardImpl::ComputerKeyboardImpl(ComputerKeyboard &owner)
    : QObject{nullptr}, m_owner(owner)
{
}

void ComputerKeyboardImpl::init() { qApp->installEventFilter(this); }

bool ComputerKeyboardImpl::eventFilter(QObject *watched, QEvent *event)
{
  const auto modifiers = QGuiApplication::queryKeyboardModifiers();
  const bool isKeyModifierPressed =
      modifiers & Qt::KeyboardModifier::ControlModifier ||
      modifiers & Qt::KeyboardModifier::AltModifier ||
      modifiers & Qt::KeyboardModifier::ShiftModifier;
  if (!isKeyModifierPressed)
  {
    if (event->type() == QEvent::KeyPress)
    {
      auto keyEvent = static_cast<const QKeyEvent *>(event);
      if (keyEvent->isAutoRepeat())
        return false;
      m_owner.onKeyPressed(keyEvent->key());
    }
    else if (event->type() == QEvent::KeyRelease)
    {
      auto keyEvent = static_cast<const QKeyEvent *>(event);
      if (keyEvent->isAutoRepeat())
        return false;
      m_owner.onKeyReleased(keyEvent->key());
    }
  }
  return QObject::eventFilter(watched, event);
}

ComputerKeyboard::ComputerKeyboard()
    : m_impl{std::make_unique<ComputerKeyboardImpl>(*this)}
{
}

void ComputerKeyboard::preInit()
{
  m_impl->init();
}

void ComputerKeyboard::onKeyPressed(char letter) { m_keyPressed.send(letter); }

void ComputerKeyboard::onKeyReleased(char letter)
{
  m_keyReleased.send(letter);
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