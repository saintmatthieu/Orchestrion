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
#include "SleepInhibitor.h"

#include <QGuiApplication>

#ifdef Q_OS_LINUX
#include <QDBusInterface>
#include <QDBusReply>
#endif

namespace dgk
{
void SleepInhibitor::init()
{
  connect(qApp, &QGuiApplication::applicationStateChanged, this,
          [this](Qt::ApplicationState state)
          {
            if (state == Qt::ApplicationActive)
              inhibit();
            else
              uninhibit();
          });

  if (QGuiApplication::applicationState() == Qt::ApplicationActive)
    inhibit();
}

void SleepInhibitor::inhibit()
{
#ifdef Q_OS_LINUX
  if (m_cookie.has_value())
    return;
  QDBusInterface iface("org.freedesktop.ScreenSaver",
                       "/org/freedesktop/ScreenSaver",
                       "org.freedesktop.ScreenSaver");
  const QDBusReply<uint> reply =
      iface.call("Inhibit", "Orchestrion", "Application is in focus");
  if (reply.isValid())
    m_cookie = reply.value();
#elif defined(Q_OS_WIN)
  SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED |
                          ES_DISPLAY_REQUIRED);
#elif defined(Q_OS_MAC)
  if (m_inhibiting)
    return;
  const kern_return_t result = IOPMAssertionCreateWithName(
      kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
      CFSTR("Orchestrion is in focus"), &m_assertionId);
  m_inhibiting = (result == kIOReturnSuccess);
#endif
}

void SleepInhibitor::uninhibit()
{
#ifdef Q_OS_LINUX
  if (!m_cookie.has_value())
    return;
  QDBusInterface iface("org.freedesktop.ScreenSaver",
                       "/org/freedesktop/ScreenSaver",
                       "org.freedesktop.ScreenSaver");
  iface.call("UnInhibit", m_cookie.value());
  m_cookie.reset();
#elif defined(Q_OS_WIN)
  SetThreadExecutionState(ES_CONTINUOUS);
#elif defined(Q_OS_MAC)
  if (!m_inhibiting)
    return;
  IOPMAssertionRelease(m_assertionId);
  m_inhibiting = false;
#endif
}
} // namespace dgk
