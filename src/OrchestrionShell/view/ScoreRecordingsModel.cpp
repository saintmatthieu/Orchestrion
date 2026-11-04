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
#include "ScoreRecordingsModel.h"

#include <project/inotationproject.h>

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrlQuery>
#include <optional>

namespace dgk
{
namespace
{
/** What the player page needs of a YouTube link, if that's what it is. */
struct YouTubeVideo
{
  QString id;
  QString start;
};

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
} // namespace

ScoreRecordingsModel::ScoreRecordingsModel(QObject *parent) : QObject(parent) {}

void ScoreRecordingsModel::load()
{
  loadBundledIds();

  projectSession()->projectChanged().onNotify(this,
                                              [this]
                                              {
                                                m_offerClosed = false;
                                                setOfferShown(false);
                                                stopWatching();
                                                update();
                                              });
  rollsLibrary()->scoresChanged().onNotify(this, [this] { update(); });
  m_end.reachedEnd().onNotify(this,
                              [this]
                              {
                                if (!m_offerClosed && !m_recordings.empty())
                                  setOfferShown(true);
                              });
  m_end.leftEnd().onNotify(this,
                           [this]
                           {
                             setOfferShown(false);
                             // The automatic player may play along with it.
                             if (!orchestrion()->player()->IsPlaying())
                               emit pauseRequested();
                           });
  m_end.start();
  update();
}

void ScoreRecordingsModel::loadBundledIds()
{
  QFile file{globalConfiguration()->appDataPath().toQString() +
             "scores/attributions.json"};
  if (!file.open(QIODevice::ReadOnly))
    return;
  const QJsonObject scores = QJsonDocument::fromJson(file.readAll()).object();
  for (auto it = scores.begin(); it != scores.end(); ++it)
    if (const QString id = it.value().toObject().value("id").toString();
        !id.isEmpty())
      m_bundledIds.insert(it.key(), id);
}

void ScoreRecordingsModel::update()
{
  std::optional<QString> id;
  if (const auto project = projectSession()->project())
  {
    const QString path = project->path().toQString();
    id = rollsLibrary()->idOf(path);
    if (!id)
      if (const auto it = m_bundledIds.constFind(QFileInfo{path}.fileName());
          it != m_bundledIds.constEnd())
        id = it.value();
  }
  m_recordings = id ? rollsLibrary()->recordings(*id)
                    : std::vector<ScoreRecording>{};
  emit recordingsChanged();
  if (m_recordings.empty())
    setOfferShown(false);
  if (m_current >= static_cast<int>(m_recordings.size()))
    stopWatching();
  // Only for a piece that may have some: nobody playing their own score
  // needs the index.
  if (id)
    rollsLibrary()->refresh();
}

void ScoreRecordingsModel::watch(int index)
{
  if (index < 0 || index >= static_cast<int>(m_recordings.size()))
    return;
  closeOffer();
  if (!youTubeVideo(m_recordings[index].url))
  {
    platformInteractive()->openUrl(m_recordings[index].url);
    return;
  }
  m_current = index;
  emit currentChanged();
}

void ScoreRecordingsModel::stopWatching()
{
  if (m_current < 0)
    return;
  m_current = -1;
  emit currentChanged();
}

void ScoreRecordingsModel::watchInBrowser()
{
  if (m_current < 0)
    return;
  const QUrl url = m_recordings[m_current].url;
  stopWatching();
  platformInteractive()->openUrl(url);
}

int ScoreRecordingsModel::currentIndex() const { return m_current; }

QUrl ScoreRecordingsModel::playerUrl() const
{
  if (m_current < 0)
    return {};
  const auto video = youTubeVideo(m_recordings[m_current].url);
  if (!video)
    return {};
  // On the website: YouTube's player wants to be told what page it is on.
  QUrl url = rollsLibrary()->siteUrl().resolved(QUrl{"player.html"});
  QUrlQuery query;
  query.addQueryItem("v", video->id);
  if (!video->start.isEmpty())
    query.addQueryItem("start", video->start);
  url.setQuery(query);
  return url;
}

void ScoreRecordingsModel::closeOffer()
{
  m_offerClosed = true;
  setOfferShown(false);
}

QStringList ScoreRecordingsModel::performers() const
{
  QStringList names;
  for (const ScoreRecording &recording : m_recordings)
    names << recording.performer;
  return names;
}

bool ScoreRecordingsModel::offerShown() const { return m_offerShown; }

void ScoreRecordingsModel::setOfferShown(bool shown)
{
  if (shown == m_offerShown)
    return;
  m_offerShown = shown;
  emit offerShownChanged();
}
} // namespace dgk
