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
#include <QObject>
#include <QTimer>
#include <memory>
#include <mutex>

namespace dgk
{
class ComputerKeyboard;
class RawKeyboard;

class ComputerKeyboardImpl : public QObject
{
  Q_OBJECT

public:
  ComputerKeyboardImpl(ComputerKeyboard &owner);
  void init();

private:
  bool eventFilter(QObject *watched, QEvent *event) override;
  /** Tells the raw keyboard, if any, whether keys play. */
  void updatePlayable();

  ComputerKeyboard &m_owner;
  /**
   * What the keys are for may change without the focus moving (the score
   * shelf shown around it): looked at again every so often.
   */
  QTimer m_playableTimer;
};

class ComputerKeyboard : public IComputerKeyboard
{
public:
  ComputerKeyboard();
  ~ComputerKeyboard() override;
  void preInit();

  /** From any thread. */
  void onKey(char letter, bool pressed);
  /** The raw keyboard, if the keys are read from one. */
  RawKeyboard *raw() const { return m_raw.get(); }

private:
  muse::async::Channel<char> keyPressed() const override;
  muse::async::Channel<char> keyReleased() const override;
  void setReceiver(Receiver) override;

  const std::unique_ptr<ComputerKeyboardImpl> m_impl;
  /** X11 only: its implementation is Linux's (see preInit). */
  std::shared_ptr<RawKeyboard> m_raw;
  std::mutex m_receiverMutex;
  Receiver m_receiver;
  /**
   * The main thread's: the channels are sent in it, through a queued call
   * from the raw keyboard's thread, which mustn't wait for the main thread,
   * as kors could make it.
   */
  QObject m_mainThreadContext;
  muse::async::Channel<char> m_keyPressed;
  muse::async::Channel<char> m_keyReleased;
};
} // namespace dgk