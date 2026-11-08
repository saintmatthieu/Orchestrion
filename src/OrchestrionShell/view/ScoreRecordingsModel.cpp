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

#include "OrchestrionShell/internal/YouTubeLinks.h"

#include <project/inotationproject.h>

#include <QUrlQuery>
#include <optional>

namespace dgk
{
ScoreRecordingsModel::ScoreRecordingsModel(QObject *parent) : QObject(parent) {}

void ScoreRecordingsModel::load()
{
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

void ScoreRecordingsModel::update()
{
  std::optional<QString> id;
  if (const auto project = projectSession()->project())
    id = rollsLibrary()->pieceIdOf(project->path().toQString());
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
