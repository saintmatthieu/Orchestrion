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
namespace
{
/**
 * The seconds of a link's t= or end= ("412" or "412s", as the Rolls index
 * writes them); empty if absent or not that.
 */
QString seconds(const QUrlQuery &query, const QString &key)
{
  QString value = query.queryItemValue(key);
  value.remove(QRegularExpression{"s$"});
  return QRegularExpression{"^[0-9]+$"}.match(value).hasMatch() ? value
                                                                : QString{};
}
} // namespace

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
  YouTubeVideo video{id, seconds(query, "t"), seconds(query, "end")};
  // YouTube ignores an end= of its own links: it's for Orchestrion's player,
  // which would stop at once at one before the start.
  if (!video.end.isEmpty() && video.end.toInt() <= video.start.toInt())
    video.end.clear();
  return video;
}

QUrl youTubeFrame(const QUrl &url)
{
  const auto video = youTubeVideo(url);
  if (!video)
    return {};
  return QUrl{"https://i.ytimg.com/vi/" + video->id + "/mqdefault.jpg"};
}
} // namespace dgk
