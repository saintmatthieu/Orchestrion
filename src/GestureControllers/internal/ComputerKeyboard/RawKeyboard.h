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
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace dgk
{
/**
 * The computer keyboard's keys, read on a thread of its own rather than from
 * Qt's key events, which come on the main thread, as late as it is busy (see
 * OrchestrionSequencer/threading.md). X11 only: XInput2's raw key events, on
 * a connection of its own. Raw events come in addition to the window's
 * ordinary ones, which Qt still gets; selecting XI_KeyPress on Qt's window
 * instead would take them from it.
 *
 * Raw events don't say which window they are for: the main thread tells
 * whether keys play (SetPlayable), which a press must be for to count. A
 * release always does, a key held as the focus moved being released all the
 * same. As with Qt's events, nothing counts while Shift, Ctrl or Alt is held.
 */
class RawKeyboard
{
public:
  /** A key's character as QKeyEvent::key() has it, pressed or released. */
  using Callback = std::function<void(char key, bool pressed)>;

  /**
   * Reads the keys from now on, calling `callback` on its thread; none if
   * there's no X display with XInput 2.1 (Qt on another platform, ...).
   */
  static std::shared_ptr<RawKeyboard> Create(Callback callback);
  ~RawKeyboard();

  RawKeyboard(const RawKeyboard &) = delete;
  RawKeyboard &operator=(const RawKeyboard &) = delete;

  /** Whether the presses play. Any thread. */
  void SetPlayable(bool playable) { m_playable = playable; }

private:
  struct Connection;
  RawKeyboard(std::unique_ptr<Connection>, Callback);
  void Run();

  const std::unique_ptr<Connection> m_connection;
  const Callback m_callback;
  std::atomic<bool> m_playable = false;
  /** Written to stop the thread. */
  int m_stopFd = -1;
  std::thread m_thread;
};
} // namespace dgk
