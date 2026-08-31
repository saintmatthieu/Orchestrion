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
#include "MacOSWindowChrome.h"

#import <AppKit/AppKit.h>

#include <QTimer>

namespace dgk
{
MacOSWindowChrome::MacOSWindowChrome(QObject *parent) : QObject{parent} {}

QWindow *MacOSWindowChrome::window() const { return m_window; }

void MacOSWindowChrome::setWindow(QWindow *window)
{
  if (m_window == window)
    return;
  if (m_window)
    m_window->disconnect(this);
  m_window = window;
  if (m_window)
    // Entering or leaving fullscreen makes AppKit (or Qt, when it recreates
    // the platform window) restyle the NSWindow, losing the custom background
    // — without this the title bar comes back grey from fullscreen. Re-apply
    // immediately and once more after the transition animation has settled.
    connect(m_window, &QWindow::visibilityChanged, this,
            [this]
            {
              apply();
              QTimer::singleShot(std::chrono::milliseconds{600}, this,
                                 [this] { apply(); });
            });
  emit windowChanged();
  apply();
}

QColor MacOSWindowChrome::color() const { return m_color; }

void MacOSWindowChrome::setColor(const QColor &color)
{
  if (m_color == color)
    return;
  m_color = color;
  emit colorChanged();
  apply();
}

void MacOSWindowChrome::apply()
{
  if (!m_window || !m_color.isValid())
    return;

  NSView *nsView = (__bridge NSView *)reinterpret_cast<void *>(m_window->winId());
  NSWindow *nsWindow = [nsView window];
  if (!nsWindow)
    return;

  // MuseScore's platform styling also sets this, but only once at startup —
  // re-assert it so a restyled/recreated NSWindow gets it back too.
  [nsWindow setTitlebarAppearsTransparent:YES];
  // With the title bar transparent, the window's background color is what
  // shows through it.
  [nsWindow setBackgroundColor:[NSColor colorWithRed:m_color.redF()
                                               green:m_color.greenF()
                                                blue:m_color.blueF()
                                               alpha:m_color.alphaF()]];
  // Light-on-dark title text and traffic lights, whatever the app theme.
  [nsWindow setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
}
} // namespace dgk
