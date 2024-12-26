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

#include "IComputerKeyboard.h"
#include "IComputerKeyboardMidiController.h"
#include <QObject>
#include <modularity/ioc.h>

namespace dgk
{
class ComputerKeyboard;

class ComputerKeyboardImpl : public QObject
{
  Q_OBJECT

public:
  ComputerKeyboardImpl(ComputerKeyboard &owner);
  void init();

private:
  bool eventFilter(QObject *watched, QEvent *event) override;
  ComputerKeyboard &m_owner;
};

class ComputerKeyboard : public IComputerKeyboard, public muse::Injectable
{
  muse::Inject<IComputerKeyboardMidiController> keyboardController;

public:
  ComputerKeyboard();
  void preInit();

  void onKeyPressed(char letter);
  void onKeyReleased(char letter);

private:
  Layout layout() const override;
  void setLayout(Layout) override;
  muse::async::Notification layoutChanged() const override;
  muse::async::Channel<char> keyPressed() const override;
  muse::async::Channel<char> keyReleased() const override;

  const std::unique_ptr<ComputerKeyboardImpl> m_impl;
  Layout m_layout;
  muse::async::Notification m_layoutChanged;
  muse::async::Channel<char> m_keyPressed;
  muse::async::Channel<char> m_keyReleased;
};
} // namespace dgk