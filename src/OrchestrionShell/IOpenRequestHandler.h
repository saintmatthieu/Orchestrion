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

#include <modularity/imoduleinterface.h>

#include <QUrl>

namespace dgk
{
/**
 * Opens what is asked for from outside the application: a score given to
 * another launch of it, a file or a link to `orchestrion://` opened in the
 * file manager or the browser.
 *
 * A link opens a score of Rolls: `orchestrion://rolls/<id>`. Only an id: a
 * link never makes the application fetch anything but from Rolls.
 *
 * Requests are carried out once the main window is ready (setReady()).
 */
class IOpenRequestHandler : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IOpenRequestHandler);

public:
  virtual ~IOpenRequestHandler() = default;

  /**
   * A local file's URL or a link; an empty URL only brings the window to
   * the front. Anything else is ignored.
   */
  virtual void open(const QUrl &url) = 0;
  /** Whether a request waits for the main window. */
  virtual bool hasPending() const = 0;
  /** The main window is ready: the requests made so far are carried out. */
  virtual void setReady() = 0;
};
} // namespace dgk
