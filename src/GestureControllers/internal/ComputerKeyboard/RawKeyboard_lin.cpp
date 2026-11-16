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
#include "RawKeyboard.h"

#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/keysym.h>

#include <cctype>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace dgk
{
/** The X connection, opened and closed on the main thread, read on its own. */
struct RawKeyboard::Connection
{
  Display *display = nullptr;
  int xinputOpcode = 0;
  /** Written to stop the thread. */
  int stopFd = -1;

  ~Connection()
  {
    if (stopFd >= 0)
      close(stopFd);
    if (display)
      XCloseDisplay(display);
  }
};

std::shared_ptr<RawKeyboard> RawKeyboard::Create(Callback callback)
{
  auto connection = std::make_unique<Connection>();
  connection->display = XOpenDisplay(nullptr);
  if (!connection->display)
    return nullptr;
  int event = 0;
  int error = 0;
  int major = 2;
  int minor = 1; // raw events delivered to any client
  if (!XQueryExtension(connection->display, "XInputExtension",
                       &connection->xinputOpcode, &event, &error) ||
      XIQueryVersion(connection->display, &major, &minor) != Success ||
      (major == 2 && minor < 1))
    return nullptr;
  unsigned char bits[XIMaskLen(XI_LASTEVENT)] = {};
  XISetMask(bits, XI_RawKeyPress);
  XISetMask(bits, XI_RawKeyRelease);
  XIEventMask mask{XIAllMasterDevices, sizeof bits, bits};
  XISelectEvents(connection->display, DefaultRootWindow(connection->display),
                 &mask, 1);
  XSync(connection->display, False);
  connection->stopFd = eventfd(0, EFD_CLOEXEC);
  return std::shared_ptr<RawKeyboard>(
      new RawKeyboard(std::move(connection), std::move(callback)));
}

RawKeyboard::RawKeyboard(std::unique_ptr<Connection> connection,
                         Callback callback)
    : m_connection{std::move(connection)}, m_callback{std::move(callback)},
      m_thread{[this] { Run(); }}
{
}

RawKeyboard::~RawKeyboard()
{
  const uint64_t one = 1;
  [[maybe_unused]] const auto written =
      write(m_connection->stopFd, &one, sizeof one);
  m_thread.join();
}

namespace
{
/**
 * The key's character, as QKeyEvent::key() has it: the symbol of the key in
 * the keyboard's layout and state (Num Lock, ...), letters upper-case; 0 for
 * one that isn't printable ASCII.
 */
char KeyOf(Display *display, unsigned keycode, const XkbStateRec &state)
{
  unsigned modsReturned = 0;
  KeySym symbol = NoSymbol;
  if (!XkbLookupKeySym(display, static_cast<KeyCode>(keycode),
                       XkbBuildCoreState(state.mods & ~ShiftMask, state.group),
                       &modsReturned, &symbol))
    return 0;
  // Latin-1's printable ASCII keysyms are their characters
  if (symbol < 0x20 || symbol > 0x7e)
  {
    // The keypad's digits, Num Lock on
    if (symbol >= XK_KP_0 && symbol <= XK_KP_9)
      return static_cast<char>('0' + (symbol - XK_KP_0));
    return 0;
  }
  return static_cast<char>(std::toupper(static_cast<int>(symbol)));
}
} // namespace

void RawKeyboard::Run()
{
  Display *const display = m_connection->display;
  pollfd fds[2] = {{ConnectionNumber(display), POLLIN, 0},
                   {m_connection->stopFd, POLLIN, 0}};
  while (true)
  {
    if (!XPending(display))
    {
      if (poll(fds, 2, -1) < 0)
        continue;
      if (fds[1].revents & POLLIN)
        return;
      if (!XPending(display))
        continue;
    }
    XEvent event;
    XNextEvent(display, &event);
    XGenericEventCookie &cookie = event.xcookie;
    if (cookie.type != GenericEvent ||
        cookie.extension != m_connection->xinputOpcode ||
        !XGetEventData(display, &cookie))
      continue;
    const bool pressed = cookie.evtype == XI_RawKeyPress;
    if (pressed || cookie.evtype == XI_RawKeyRelease)
    {
      const auto *raw = static_cast<const XIRawEvent *>(cookie.data);
      const bool repeat = raw->flags & XIKeyRepeat;
      if (!repeat && (!pressed || m_playable))
      {
        XkbStateRec state{};
        XkbGetState(display, XkbUseCoreKbd, &state);
        const bool modified = state.mods & (ShiftMask | ControlMask | Mod1Mask);
        if (const char key = KeyOf(display, raw->detail, state);
            key && !modified)
          m_callback(key, pressed);
      }
    }
    XFreeEventData(display, &cookie);
  }
}
} // namespace dgk
