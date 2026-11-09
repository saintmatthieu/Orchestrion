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
#include <optional>

namespace dgk
{
/** One or the other is set. */
struct OpenRequest
{
  QString rollsId;
  QString filePath;
};

/**
 * What was asked of Orchestrion from outside, to open while it runs or as it
 * starts: an orchestrion:// link, or a file. Only the last one asked counts,
 * since one score is open at a time. It waits here until the window is up.
 */
class IOpenRequests : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IOpenRequests);

public:
  virtual ~IOpenRequests() = default;

  /** A link, or a file's absolute path. Anything else is logged and dropped. */
  virtual void submit(const QString &linkOrPath) = 0;

  virtual bool hasPending() const = 0;
  /**
   * Nothing until setReady(): opening a score closes the dialogs, and at
   * startup that would be the progress of the audio plugin scan.
   */
  virtual std::optional<OpenRequest> take() = 0;
  /** The application's startup is over. */
  virtual void setReady() = 0;
  virtual muse::async::Notification requested() const = 0;
};
} // namespace dgk
