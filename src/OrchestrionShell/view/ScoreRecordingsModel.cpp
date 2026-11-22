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
#include <algorithm>
#include <cmath>
#include <chrono>
#include <optional>

namespace dgk
{
ScoreRecordingsModel::ScoreRecordingsModel(QObject *parent)
    : QObject(parent), m_end{std::chrono::seconds{3}}
{
  m_activeSyncBeatTimer.setSingleShot(true);
  connect(&m_activeSyncBeatTimer, &QTimer::timeout, this,
          [this] { updateActiveSyncBeat(); });
  m_replayTimer.setSingleShot(true);
  m_replayTimer.setInterval(400);
  connect(&m_replayTimer, &QTimer::timeout, this,
          [this]
          {
            if (!m_session || m_replayBeat < 0 ||
                m_replayBeat >= syncCount())
              return;
            constexpr double leadIn = 1.5;
            emit replayRequested(std::max(
                0.0, syncBeatSeconds(m_replayBeat) - leadIn));
          });
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
          if (m_session && reason == JumpReason::Click)
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
    const mu::engraving::Score &score = *project->masterNotation()->masterScore();
    if (const auto sync = ReadRecordingSync(path, PlayedBars(score)))
    {
      m_timeline = MakeRecordingTimeline(score, *sync);
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
  if (m_session)
    updateActiveSyncBeat();
  if (m_current < 0 || !m_timeline || m_session)
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
  if (m_session || m_recordings.empty())
    return;
  if (m_current < 0)
    watch(0);
  const auto project = projectSession()->project();
  const QString path = syncPath();
  if (m_current < 0 || !project || path.isEmpty())
    return; // Not a video the app plays.

  std::vector<PlayedBar> bars =
      PlayedBars(*project->masterNotation()->masterScore());
  if (bars.empty())
    return;
  RecordingSync sync = ReadRecordingSync(path, bars).value_or(RecordingSync{});
  if (sync.scoreVersion.isEmpty())
    if (const RollsScore *score = rollsLibrary()->score(*m_pieceId);
        score && !rollsLibrary()->hasUpdate(*m_pieceId))
      sync.scoreVersion = score->version;
  m_session.emplace(std::move(bars), std::move(sync));
  m_session->SetPrecision(m_precision);
  // What is tapped is followed once done, not while tapping.
  orchestrion()->player()->FollowRecording(nullptr);
  emit syncChanged();
  updateActiveSyncBeat();
}

void ScoreRecordingsModel::syncEdited()
{
  const QString path = syncPath();
  if (!path.isEmpty() && !WriteRecordingSync(path, m_session->Sync()))
    LOGE() << "Could not save the sync: " << path;
  emit syncChanged();
  updateActiveSyncBeat();
}

void ScoreRecordingsModel::tapBeat()
{
  if (m_session && m_videoPlaying && m_session->Tap(videoSecondsNow()))
    syncEdited();
}

void ScoreRecordingsModel::undoSync()
{
  if (m_session && m_session->Undo())
    syncEdited();
}

void ScoreRecordingsModel::syncFrom(int writtenTick)
{
  // Of a repeat's passes, the one nearest where the video is.
  int near = 0;
  if (m_activeSyncBeat >= 0)
    near = m_session->Position(m_activeSyncBeat);
  else if (const auto next = m_session->Next())
    near = *next;
  const auto bar = m_session->BarAt(writtenTick, near);
  if (!bar)
    return;
  if (const auto from = m_session->TapFrom(*bar))
    emit seekRequested(m_startSeconds + *from);
  emit syncChanged();
}

void ScoreRecordingsModel::finishSync()
{
  if (!m_session)
    return;
  m_session.reset();
  m_replayTimer.stop();
  emit syncChanged();
  updateActiveSyncBeat();
  loadSync();
}

bool ScoreRecordingsModel::syncing() const { return m_session.has_value(); }

int ScoreRecordingsModel::syncCount() const
{
  return m_session ? static_cast<int>(m_session->Sync().points.size()) : 0;
}

int ScoreRecordingsModel::syncPrecision() const
{
  return m_precision == RecordingSyncSession::Precision::beats ? 0 : 1;
}

void ScoreRecordingsModel::setSyncPrecision(int precision)
{
  m_precision = precision == 0 ? RecordingSyncSession::Precision::beats
                               : RecordingSyncSession::Precision::subdivisions;
  if (m_session)
    m_session->SetPrecision(m_precision);
  emit syncChanged();
}

QString ScoreRecordingsModel::syncNext() const
{
  if (!m_session)
    return {};
  const auto next = m_session->Next();
  return next ? positionName(*next) : tr("the end");
}

QString ScoreRecordingsModel::positionName(int position) const
{
  const auto place = m_session->PlaceOf(position);
  const PlayedBar &bar = m_session->Bars()[place.bar];
  QString name = bar.pickup ? tr("the pickup") : tr("bar %1").arg(bar.number);
  if (bar.pass > 1)
    name = tr("%1 (time %2)").arg(name).arg(bar.pass);
  if (place.subdivision > 1)
    //: e.g. "bar 12, beat 3 (2/3)": the second third of the beat
    return tr("%1, beat %2 (%3/%4)")
        .arg(name)
        .arg(place.beat)
        .arg(place.subdivision)
        .arg(place.subdivisions);
  //: e.g. "bar 12, beat 3"
  return tr("%1, beat %2").arg(name).arg(place.beat);
}

QVariantList ScoreRecordingsModel::syncBeats() const
{
  QVariantList beats;
  if (!m_session)
    return beats;
  const RecordingSync &sync = m_session->Sync();
  for (const RecordingSync::Point &point : sync.points)
  {
    const PlayedBar &bar = m_session->Bars()[point.bar];
    beats.append(QVariantMap{{"tick", bar.writtenTick + point.tick},
                             {"onBeat", point.tick % bar.beatTicks == 0}});
  }
  return beats;
}

QVariantList ScoreRecordingsModel::selectedSyncBeats() const
{
  QVariantList selected;
  if (m_session)
    for (const int beat : m_session->Selection())
      selected.append(beat);
  return selected;
}

QString ScoreRecordingsModel::syncBeatName(int beat) const
{
  if (!m_session || beat < 0 || beat >= syncCount())
    return {};
  return positionName(m_session->Position(beat));
}

double ScoreRecordingsModel::syncBeatSeconds(int beat) const
{
  if (!m_session || beat < 0 || beat >= syncCount())
    return 0.0;
  return m_startSeconds + m_session->Sync().points[beat].seconds;
}

double ScoreRecordingsModel::syncBeatMinimum(int beat) const
{
  if (!m_session || beat < 0 || beat >= syncCount())
    return 0.0;
  return m_startSeconds + m_session->Minimum(beat);
}

double ScoreRecordingsModel::syncBeatMaximum(int beat) const
{
  if (!m_session || beat < 0 || beat >= syncCount())
    return 0.0;
  return m_startSeconds + m_session->Maximum(beat);
}

void ScoreRecordingsModel::setSyncBeatSeconds(int beat, double seconds)
{
  if (!m_session || beat < 0 || beat >= syncCount())
    return;
  m_session->SetSeconds(beat, seconds - m_startSeconds);
  syncEdited();
  m_replayBeat = beat;
  m_replayTimer.start();
}

void ScoreRecordingsModel::selectSyncBeat(int beat, bool add)
{
  if (!m_session)
    return;
  m_session->Select(beat, add);
  emit syncChanged();
}

void ScoreRecordingsModel::extendSyncSelection(int direction)
{
  if (!m_session)
    return;
  m_session->ExtendSelection(direction, m_activeSyncBeat);
  emit syncChanged();
}

bool ScoreRecordingsModel::clearSyncSelection()
{
  if (!m_session || !m_session->ClearSelection())
    return false;
  emit syncChanged();
  return true;
}

void ScoreRecordingsModel::deleteSyncSelection()
{
  if (m_session && m_session->DeleteSelection())
    syncEdited();
}

double ScoreRecordingsModel::videoSecondsNow() const
{
  double seconds = m_videoSeconds;
  if (m_videoPlaying)
    seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                             m_videoTime)
                   .count() *
               m_videoRate;
  return seconds - m_startSeconds;
}

void ScoreRecordingsModel::updateActiveSyncBeat()
{
  m_activeSyncBeatTimer.stop();
  int active = -1;
  if (m_session && !m_session->Sync().points.empty())
  {
    const double now = videoSecondsNow();
    active = m_session->AnchorAt(now);
    // On to the next when the video gets there.
    const auto &points = m_session->Sync().points;
    if (m_videoPlaying && active + 1 < static_cast<int>(points.size()))
      m_activeSyncBeatTimer.start(static_cast<int>(std::ceil(
          (points[active + 1].seconds - now) / m_videoRate * 1000.0)));
  }
  if (active == m_activeSyncBeat)
    return;
  m_activeSyncBeat = active;
  emit activeSyncBeatChanged();
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
