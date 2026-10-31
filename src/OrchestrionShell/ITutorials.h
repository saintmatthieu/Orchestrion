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

#include <async/notification.h>
#include <modularity/imoduleinterface.h>

#include <QString>
#include <vector>

namespace dgk
{
/**
 * The tutorials: the testflow scripts installed with the application, but
 * for those marked `@testflow test`. One runs in the user's session, on the
 * score that is open, showing what it does with the demo keys and cursor; Esc
 * stops it. The application stays open after it.
 */
class ITutorials : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(ITutorials);

public:
  struct Tutorial
  {
    QString path;
    QString title;
    QString description;
  };

  virtual ~ITutorials() = default;

  virtual const std::vector<Tutorial> &tutorials() const = 0;

  /**
   * Runs it, on the score that is open, or on a bundled one if none is.
   */
  virtual void run(const QString &path) = 0;
  virtual void stop() = 0;
  virtual bool running() const = 0;
  virtual muse::async::Notification runningChanged() const = 0;
};
} // namespace dgk
