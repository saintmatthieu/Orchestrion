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
#include "OrchestrionCommon/ThreadPriority.h"
#include "RawKeyboard.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <bitset>
#include <cctype>
#include <future>

namespace dgk
{
/**
 * A message-only window, made and destroyed by the thread, which gets the
 * keyboard's raw input whatever window has the focus (RIDEV_INPUTSINK).
 *
 * Raw input goes to one window per process and device class: should Qt, or
 * anything else in the process, register for the keyboard's too, one of them
 * would lose it.
 */
struct RawKeyboard::Connection
{
  /** The main thread's, whose keyboard layout the characters are in. */
  DWORD mainThreadId = 0;
  /** Whether the thread could register for the keyboard's raw input. */
  std::promise<bool> started;
  /** Set by the thread before `started`; WM_CLOSE posted to it stops it. */
  HWND window = nullptr;
};

std::shared_ptr<RawKeyboard> RawKeyboard::Create(Callback callback)
{
  auto connection = std::make_unique<Connection>();
  connection->mainThreadId = GetCurrentThreadId();
  auto started = connection->started.get_future();
  std::shared_ptr<RawKeyboard> keyboard(
      new RawKeyboard(std::move(connection), std::move(callback)));
  if (!started.get())
    return nullptr; // the thread is over, the destructor joins it
  return keyboard;
}

RawKeyboard::RawKeyboard(std::unique_ptr<Connection> connection,
                         Callback callback)
    : m_connection{std::move(connection)}, m_callback{std::move(callback)},
      m_thread{[this] { Run(); }}
{
}

RawKeyboard::~RawKeyboard()
{
  if (m_connection->window)
    PostMessageW(m_connection->window, WM_CLOSE, 0, 0);
  m_thread.join();
}

namespace
{
/**
 * The key's character, as QKeyEvent::key() has it: unshifted, in the main
 * thread's keyboard layout, letters upper-case; 0 for one that isn't
 * printable ASCII (as on X11).
 */
char KeyOf(USHORT virtualKey, HKL layout)
{
  // The keypad's digits, Num Lock on
  if (virtualKey >= VK_NUMPAD0 && virtualKey <= VK_NUMPAD9)
    return static_cast<char>('0' + (virtualKey - VK_NUMPAD0));
  // The high bit is set for a dead key
  const UINT character = MapVirtualKeyExW(virtualKey, MAPVK_VK_TO_CHAR, layout);
  if (character < 0x20 || character > 0x7e)
    return 0;
  return static_cast<char>(std::toupper(static_cast<int>(character)));
}

bool IsDown(int virtualKey) { return GetAsyncKeyState(virtualKey) & 0x8000; }
} // namespace

void RawKeyboard::Run()
{
  raiseTimedThreadPriority();
  Connection &connection = *m_connection;
  // "Message" is the system's class for message-only windows.
  const HWND window = CreateWindowExW(0, L"Message", nullptr, 0, 0, 0, 0, 0,
                                      HWND_MESSAGE, nullptr, nullptr, nullptr);
  RAWINPUTDEVICE device{};
  device.usUsagePage = 0x01; // generic desktop
  device.usUsage = 0x06;     // keyboard
  device.dwFlags = RIDEV_INPUTSINK;
  device.hwndTarget = window;
  if (!window || !RegisterRawInputDevices(&device, 1, sizeof device))
  {
    if (window)
      DestroyWindow(window);
    connection.started.set_value(false);
    return;
  }
  connection.window = window;
  connection.started.set_value(true);

  // Raw input says nothing of repeats: they are makes of a key already down.
  std::bitset<256> down;
  MSG message;
  while (GetMessageW(&message, nullptr, 0, 0) > 0)
  {
    if (message.message == WM_CLOSE)
      break;
    if (message.message == WM_INPUT)
    {
      RAWINPUT input;
      UINT size = sizeof input;
      if (GetRawInputData(reinterpret_cast<HRAWINPUT>(message.lParam),
                          RID_INPUT, &input, &size,
                          sizeof(RAWINPUTHEADER)) != static_cast<UINT>(-1) &&
          input.header.dwType == RIM_TYPEKEYBOARD)
      {
        const RAWKEYBOARD &key = input.data.keyboard;
        // 0xff: a fake key, part of an escaped sequence
        if (key.VKey != 0 && key.VKey < 0xff)
        {
          const bool pressed = !(key.Flags & RI_KEY_BREAK);
          const bool repeat = pressed && down[key.VKey];
          down[key.VKey] = pressed;
          if (!repeat && (!pressed || m_playable))
          {
            const bool modified =
                IsDown(VK_SHIFT) || IsDown(VK_CONTROL) || IsDown(VK_MENU);
            if (const char character =
                    KeyOf(key.VKey, GetKeyboardLayout(connection.mainThreadId));
                character && (!pressed || !modified))
              // A release counts whatever is held: its key was pressed
              // without.
              m_callback(character, pressed);
          }
        }
      }
    }
    // WM_INPUT's DefWindowProc frees the input's data.
    DispatchMessageW(&message);
  }
  DestroyWindow(window);
}
} // namespace dgk
