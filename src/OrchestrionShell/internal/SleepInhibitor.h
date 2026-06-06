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

#include <QObject>
#include <optional>

#ifdef Q_OS_WIN
#include <windows.h>
#elif defined(Q_OS_MAC)
#include <IOKit/pwr_mgt/IOPMLib.h>
#endif

namespace dgk
{
class SleepInhibitor : public QObject
{
  Q_OBJECT
public:
  void init();

private:
  void inhibit();
  void uninhibit();

#ifdef Q_OS_LINUX
  std::optional<uint> m_cookie;
#elif defined(Q_OS_MAC)
  IOPMAssertionID m_assertionId = 0;
  bool m_inhibiting = false;
#endif
};
} // namespace dgk
