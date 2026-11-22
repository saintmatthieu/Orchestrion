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

#include "MuseScoreShell/OrchestrionActionIds.h"
#include "OrchestrionShell/internal/RecordingSync.h"
#include "OrchestrionShell/internal/YouTubeLinks.h"

#include <log.h>
#include <engraving/dom/masterscore.h>
#include <notation/imasternotation.h>
#include <project/inotationproject.h>

#include <QUrlQuery>
#include <chrono>
#include <optional>

namespace dgk
{
ScoreRecordingsModel::ScoreRecordingsModel(QObject *parent)
    : QObject(parent), m_end{std::chrono::seconds{3}}
{
}

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
  dispatcher()->reg(this, actionIds::syncVideo, [this] { startSync(); });
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this]
                                             {
                                               subscribePlayer();
                                               orchestrion()->player()->FollowRecording(
                                                   m_timeline);
                                             });
  subscribePlayer();
  update();
}

void ScoreRecordingsModel::subscribePlayer()
{
  // Syncing, a bar clicked is the next to tap.
  if (const IOrchestrionSequencerPtr sequencer = orchestrion()->sequencer())
    sequencer->AboutToJumpPosition().onReceive(
        this,
        [this](int tick, JumpReason reason)
        {
          if (m_syncing && reason == JumpReason::Click)
            syncFrom(tick);
        },
        muse::async::Asyncable::Mode::SetReplace);
  const IOrchestrionPlayerPtr player = orchestrion()->player();
  player->RecordingSeekRequested().onReceive(
      this,
      [this](double seconds) { emit seekRequested(m_startSeconds + seconds); },
      muse::async::Asyncable::Mode::SetReplace);
  player->PlayingChanged().onNotify(
      this,
      [this]
      {
        const IOrchestrionPlayerPtr player = orchestrion()->player();
        // Its own playing, or the stop of its following the video (the
        // stop button): the video stops too.
        if ((player->IsPlaying() && !player->IsFollowing()) ||
            (m_following && !player->IsPlaying()))
          emit pauseRequested();
        m_following = player->IsFollowing();
      },
      muse::async::Asyncable::Mode::SetReplace);
}

void ScoreRecordingsModel::update()
{
  std::optional<QString> id;
  if (const auto project = projectSession()->project())
    id = rollsLibrary()->pieceIdOf(project->path().toQString());
  m_pieceId = id;
  m_recordings = id ? rollsLibrary()->recordings(*id)
                    : std::vector<ScoreRecording>{};
  emit recordingsChanged();
  emit thumbnailChanged();
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
  loadSync();
  emit currentChanged();
  emit thumbnailChanged();
}

void ScoreRecordingsModel::loadSync()
{
  m_timeline.reset();
  m_startSeconds = 0.0;
  const auto video = m_current >= 0 ? youTubeVideo(m_recordings[m_current].url)
                                    : std::nullopt;
  if (video)
    m_startSeconds = video->start.toDouble();
  const auto project = projectSession()->project();
  if (video && m_pieceId && project)
  {
    const QString path =
        RecordingSyncPath(globalConfiguration()->userAppDataPath().toQString(),
                          *m_pieceId, video->id);
    if (const auto sync = ReadRecordingSync(path))
    {
      m_timeline = MakeRecordingTimeline(
          *project->masterNotation()->masterScore(), *sync);
      if (!m_timeline)
        LOGW() << "The sync doesn't fit the score: " << path;
    }
  }
  orchestrion()->player()->FollowRecording(m_timeline);
}

void ScoreRecordingsModel::reportPlayer(double seconds, double rate, int state)
{
  constexpr int playing = 1;
  m_videoSeconds = seconds;
  m_videoTime = std::chrono::steady_clock::now();
  m_videoRate = rate > 0.0 ? rate : 1.0;
  m_videoPlaying = state == playing;
  if (m_current < 0 || !m_timeline || m_syncing)
    return;
  orchestrion()->player()->SetRecordingClock(
      seconds - m_startSeconds, m_videoTime, m_videoRate, m_videoPlaying);
}

QString ScoreRecordingsModel::syncPath() const
{
  const auto video = m_current >= 0 ? youTubeVideo(m_recordings[m_current].url)
                                    : std::nullopt;
  if (!video || !m_pieceId)
    return {};
  return RecordingSyncPath(globalConfiguration()->userAppDataPath().toQString(),
                           *m_pieceId, video->id);
}

void ScoreRecordingsModel::startSync()
{
  if (m_syncing || m_recordings.empty())
    return;
  if (m_current < 0)
    watch(0);
  const auto project = projectSession()->project();
  const QString path = syncPath();
  if (m_current < 0 || !project || path.isEmpty())
    return; // Not a video the app plays.

  m_bars = PlayedBars(*project->masterNotation()->masterScore());
  if (m_bars.empty())
    return;
  m_sync = ReadRecordingSync(path).value_or(RecordingSync{});
  if (m_sync.scoreVersion.isEmpty())
    if (const RollsScore *score = rollsLibrary()->score(*m_pieceId);
        score && !rollsLibrary()->hasUpdate(*m_pieceId))
      m_sync.scoreVersion = score->version;
  m_nextBar = 0;
  m_nextBeat = 1;
  if (!m_sync.points.empty())
  {
    m_nextBar = m_sync.points.back().bar;
    m_nextBeat = m_sync.points.back().beat;
    advanceSync();
  }
  // What is tapped is followed once done, not while tapping.
  orchestrion()->player()->FollowRecording(nullptr);
  m_syncing = true;
  emit syncChanged();
}

void ScoreRecordingsModel::advanceSync()
{
  if (m_nextBar >= static_cast<int>(m_bars.size()))
    return;
  if (++m_nextBeat > m_bars[m_nextBar].beats)
  {
    ++m_nextBar;
    m_nextBeat = 1;
  }
}

void ScoreRecordingsModel::tapBeat()
{
  if (!m_syncing || !m_videoPlaying ||
      m_nextBar >= static_cast<int>(m_bars.size()))
    return;
  const double elapsed = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - m_videoTime)
                             .count();
  const double seconds =
      m_videoSeconds + elapsed * m_videoRate - m_startSeconds;
  if (!m_sync.points.empty() && seconds <= m_sync.points.back().seconds)
    return;
  m_sync.points.push_back({m_nextBar, m_nextBeat, seconds});
  advanceSync();
  saveSync();
  emit syncChanged();
}

void ScoreRecordingsModel::undoBeat()
{
  if (!m_syncing || m_sync.points.empty())
    return;
  m_nextBar = m_sync.points.back().bar;
  m_nextBeat = m_sync.points.back().beat;
  m_sync.points.pop_back();
  saveSync();
  emit syncChanged();
}

void ScoreRecordingsModel::syncFrom(int writtenTick)
{
  // The bar clicked, among the bars played: ahead of the last tapped if it
  // is played again there, else the last time before.
  const int last = m_sync.points.empty() ? -1 : m_sync.points.back().bar;
  std::optional<int> behind, ahead;
  for (int i = 0; i < static_cast<int>(m_bars.size()); ++i)
  {
    const PlayedBar &bar = m_bars[i];
    if (writtenTick < bar.writtenTick ||
        writtenTick >= bar.writtenTick + bar.length)
      continue;
    if (i > last)
    {
      if (!ahead)
        ahead = i;
    }
    else
      behind = i;
  }
  if (ahead)
    m_nextBar = *ahead;
  else if (behind)
  {
    // Tapped again from there: the video goes back to just before.
    m_nextBar = *behind;
    std::erase_if(m_sync.points, [this](const RecordingSync::Point &point)
                  { return point.bar >= m_nextBar; });
    constexpr double leadIn = 2.0;
    const double from =
        m_sync.points.empty() ? 0.0 : m_sync.points.back().seconds - leadIn;
    emit seekRequested(m_startSeconds + std::max(0.0, from));
    saveSync();
  }
  else
    return;
  m_nextBeat = 1;
  emit syncChanged();
}

void ScoreRecordingsModel::saveSync()
{
  const QString path = syncPath();
  if (!path.isEmpty() && !WriteRecordingSync(path, m_sync))
    LOGE() << "Could not save the sync: " << path;
}

void ScoreRecordingsModel::finishSync()
{
  if (!m_syncing)
    return;
  m_syncing = false;
  saveSync();
  emit syncChanged();
  loadSync();
}

bool ScoreRecordingsModel::syncing() const { return m_syncing; }

int ScoreRecordingsModel::syncCount() const
{
  return static_cast<int>(m_sync.points.size());
}

QString ScoreRecordingsModel::syncNext() const
{
  if (!m_syncing)
    return {};
  if (m_nextBar >= static_cast<int>(m_bars.size()))
    return tr("the end");
  const PlayedBar &bar = m_bars[m_nextBar];
  QString name =
      bar.pickup ? tr("the pickup") : tr("bar %1").arg(bar.number);
  if (bar.pass > 1)
    name = tr("%1 (time %2)").arg(name).arg(bar.pass);
  //: e.g. "bar 12, beat 3"
  return tr("%1, beat %2").arg(name).arg(m_nextBeat);
}

void ScoreRecordingsModel::stopWatching()
{
  if (m_current < 0)
    return;
  finishSync();
  m_current = -1;
  loadSync();
  emit currentChanged();
  emit thumbnailChanged();
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

QUrl ScoreRecordingsModel::thumbnailUrl() const
{
  if (m_current >= 0)
    if (const QUrl url = youTubeFrame(m_recordings[m_current].url); !url.isEmpty())
      return url;
  for (const ScoreRecording &recording : m_recordings)
    if (const QUrl url = youTubeFrame(recording.url); !url.isEmpty())
      return url;
  return {};
}

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
  if (!video->end.isEmpty())
    query.addQueryItem("end", video->end);
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
