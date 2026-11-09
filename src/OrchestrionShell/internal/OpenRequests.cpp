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
#include "OpenRequests.h"
#include "OrchestrionShell/RollsLink.h"

#include <log.h>

#include <QFileInfo>
#include <utility>

namespace dgk
{
void OpenRequests::submit(const QString &linkOrPath)
{
  OpenRequest request;
  if (isOrchestrionLink(linkOrPath))
  {
    const std::optional<QString> id = rollsIdOfLink(linkOrPath);
    if (!id)
    {
      LOGW() << "Ignoring the link " << linkOrPath.left(200);
      return;
    }
    request.rollsId = *id;
  }
  else
  {
    const QFileInfo file{linkOrPath};
    if (!file.isAbsolute() || !file.isFile())
    {
      LOGW() << "Ignoring the request to open " << linkOrPath.left(200);
      return;
    }
    request.filePath = file.absoluteFilePath();
  }
  m_pending = std::move(request);
  m_requested.notify();
}

bool OpenRequests::hasPending() const { return m_pending.has_value(); }

std::optional<OpenRequest> OpenRequests::take()
{
  if (!m_ready)
    return std::nullopt;
  return std::exchange(m_pending, std::nullopt);
}

void OpenRequests::setReady()
{
  m_ready = true;
  if (m_pending)
    m_requested.notify();
}

muse::async::Notification OpenRequests::requested() const
{
  return m_requested;
}
} // namespace dgk
