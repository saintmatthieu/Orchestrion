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
#include "YouTubeLinks.h"

#include <QRegularExpression>
#include <QUrlQuery>

namespace dgk
{
std::optional<YouTubeVideo> youTubeVideo(const QUrl &url)
{
  const QString host = url.host().remove(QRegularExpression{"^(www|m)\\."});
  const QUrlQuery query{url};
  QString id;
  if (host == "youtu.be")
    id = url.path().mid(1);
  else if (host == "youtube.com" && url.path() == "/watch")
    id = query.queryItemValue("v");
  if (!QRegularExpression{"^[A-Za-z0-9_-]+$"}.match(id).hasMatch())
    return std::nullopt;
  // t=412 or t=412s
  QString start = query.queryItemValue("t");
  start.remove(QRegularExpression{"s$"});
  return YouTubeVideo{id, QRegularExpression{"^[0-9]+$"}.match(start).hasMatch()
                              ? start
                              : QString{}};
}

QUrl youTubeFrame(const QUrl &url)
{
  const auto video = youTubeVideo(url);
  if (!video)
    return {};
  return QUrl{"https://i.ytimg.com/vi/" + video->id + "/mqdefault.jpg"};
}
} // namespace dgk
