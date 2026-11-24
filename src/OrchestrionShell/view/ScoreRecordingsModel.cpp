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

#include <engraving/dom/masterscore.h>
#include <log.h>
#include <notation/imasternotation.h>
#include <project/inotationproject.h>

#include <QCoreApplication>
#include <QKeyEvent>
#include <QUrlQuery>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <optional>

namespace dgk
{
ScoreRecordingsModel::ScoreRecordingsModel(QObject *parent)
    : QObject(parent), m_end{std::chrono::seconds{3}}
{
  qApp->installEventFilter(this);
  m_activeSyncBeatTimer.setSingleShot(true);
  connect(&m_activeSyncBeatTimer, &QTimer::timeout, this,
          [this] { updateActiveSyncBeat(); });
  m_followSpanTimer.setSingleShot(true);
  connect(&m_followSpanTimer, &QTimer::timeout, this,
          [this] { updateFollowSpan(); });
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
  orchestrion()->sequencerChanged().onNotify(
      this,
      [this]
      {
        subscribePlayer();
        orchestrion()->player()->FollowRecording(m_timeline);
        orchestrion()->player()->SetTempoCurve(m_tempoCurve);
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
        if (m_following != player->IsFollowing())
        {
          m_following = player->IsFollowing();
          emit followingChanged();
        }
      },
      muse::async::Asyncable::Mode::SetReplace);
}

void ScoreRecordingsModel::update()
{
  std::optional<QString> id;
  if (const auto project = projectSession()->project())
    id = rollsLibrary()->pieceIdOf(project->path().toQString());
  m_pieceId = id;
  m_recordings =
      id ? rollsLibrary()->recordings(*id) : std::vector<ScoreRecording>{};
  emit recordingsChanged();
  emit thumbnailChanged();
  if (m_recordings.empty())
    setOfferShown(false);
  if (m_current >= static_cast<int>(m_recordings.size()))
    stopWatching();
  updateTempoCurve();
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
  // Until the player tells: at the start, not playing.
  m_videoClock = {};
  const auto video =
      m_current >= 0 ? youTubeVideo(m_recordings[m_current].url) : std::nullopt;
  if (video)
    m_startSeconds = video->start.toDouble();
  m_videoClock.Tell(m_startSeconds, std::chrono::steady_clock::now(), 1.0,
                    false);
  const auto project = projectSession()->project();
  if (video && m_pieceId && project)
  {
    const QString path =
        RecordingSyncPath(globalConfiguration()->userAppDataPath().toQString(),
                          *m_pieceId, video->id);
    const mu::engraving::Score &score =
        *project->masterNotation()->masterScore();
    if (const auto sync = ReadRecordingSync(path, PlayedBars(score)))
    {
      m_timeline = MakeRecordingTimeline(score, *sync);
      if (!m_timeline)
        LOGW() << "The sync doesn't fit the score: " << path;
    }
  }
  orchestrion()->player()->FollowRecording(m_timeline);
  updateFollowSpan();
  updateTempoCurve();
}

void ScoreRecordingsModel::updateTempoCurve()
{
  std::shared_ptr<const RecordingTimeline> curve = m_timeline;
  const auto project = projectSession()->project();
  if (!curve && m_pieceId && project)
  {
    const mu::engraving::Score &score =
        *project->masterNotation()->masterScore();
    const std::vector<PlayedBar> bars = PlayedBars(score);
    for (const ScoreRecording &recording : m_recordings)
      if (const auto video = youTubeVideo(recording.url))
        if (const auto sync = ReadRecordingSync(
                RecordingSyncPath(
                    globalConfiguration()->userAppDataPath().toQString(),
                    *m_pieceId, video->id),
                bars))
          if ((curve = MakeRecordingTimeline(score, *sync)))
            break;
  }
  m_tempoCurve = curve;
  orchestrion()->player()->SetTempoCurve(m_tempoCurve);
}

void ScoreRecordingsModel::reportPlayer(double seconds, double rate, int state)
{
  constexpr int playing = 1;
  const auto now = std::chrono::steady_clock::now();
  m_videoClock.Tell(seconds, now, rate > 0.0 ? rate : 1.0, state == playing);
  if (m_session)
    updateActiveSyncBeat();
  updateFollowSpan();
  if (m_current < 0 || !m_timeline || m_session)
    return;
  orchestrion()->player()->SetRecordingClock(
      m_videoClock.SecondsAt(now) - m_startSeconds, now, m_videoClock.Rate(),
      m_videoClock.Playing());
}

QString ScoreRecordingsModel::syncPath() const
{
  const auto video =
      m_current >= 0 ? youTubeVideo(m_recordings[m_current].url) : std::nullopt;
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
  // The version tapped on: the one downloaded, whatever the index has since.
  if (sync.scoreVersion.isEmpty())
    sync.scoreVersion =
        rollsLibrary()->downloadedVersion(*m_pieceId).value_or(QString{});
  m_session.emplace(std::move(bars), std::move(sync));
  m_leastTapDelayMs.reset();
  // What is tapped is followed once done, not while tapping.
  orchestrion()->player()->FollowRecording(nullptr);
  emit syncChanged();
  updateActiveSyncBeat();
  updateFollowSpan();
}

void ScoreRecordingsModel::syncEdited()
{
  const QString path = syncPath();
  if (!path.isEmpty() && !WriteRecordingSync(path, m_session->Sync()))
    LOGE() << "Could not save the sync: " << path;
  emit syncChanged();
  updateActiveSyncBeat();
}

bool ScoreRecordingsModel::eventFilter(QObject *, QEvent *event)
{
  // A key bound to a shortcut comes as a shortcut override first, and may
  // never come as a press.
  if (m_session && (event->type() == QEvent::KeyPress ||
                    event->type() == QEvent::ShortcutOverride))
  {
    const auto *key = static_cast<QKeyEvent *>(event);
    if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) &&
        !key->isAutoRepeat())
      m_returnPressedAt = key->timestamp();
  }
  return false;
}

void ScoreRecordingsModel::tapBeat()
{
  // When the key was pressed, rather than when the tap gets here, which is
  // later by however busy the app is: by how much longer than the least it
  // has taken (the key's time counts from where its system likes).
  double late = 0.0;
  if (m_returnPressedAt)
  {
    const double nowMs =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    const double delay = nowMs - static_cast<double>(*m_returnPressedAt);
    m_leastTapDelayMs = std::min(m_leastTapDelayMs.value_or(delay), delay);
    constexpr double longest = 200.0; // beyond, something else is amiss
    late = std::min(delay - *m_leastTapDelayMs, longest) / 1000.0 *
           m_videoClock.Rate();
    m_returnPressedAt.reset();
  }
  if (m_session && m_videoClock.Playing() &&
      m_session->Tap(videoSecondsNow() - late))
    syncEdited();
}

void ScoreRecordingsModel::undoSync()
{
  if (m_session && m_session->Undo())
    syncEdited();
}

void ScoreRecordingsModel::syncFrom(int writtenTick)
{
  // Of a repeat's passes, the one nearest where the video is: at the anchor
  // it is at, or before them all, at the score's start.
  const int near =
      m_activeSyncBeat >= 0 ? m_session->Position(m_activeSyncBeat) : 0;
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
  emit syncChanged();
  updateActiveSyncBeat();
  loadSync();
}

bool ScoreRecordingsModel::syncing() const { return m_session.has_value(); }

int ScoreRecordingsModel::syncCount() const
{
  return m_session ? static_cast<int>(m_session->Sync().points.size()) : 0;
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
    beats.append(QVariantMap{{"tick", bar.writtenTick + point.tick}});
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

void ScoreRecordingsModel::selectAllSyncBeats()
{
  if (!m_session)
    return;
  m_session->SelectAll();
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
  return m_videoClock.SecondsAt(std::chrono::steady_clock::now()) -
         m_startSeconds;
}

void ScoreRecordingsModel::updateFollowSpan()
{
  m_followSpanTimer.stop();
  QVariantMap span;
  if (m_timeline && !m_session && m_current >= 0)
  {
    const std::vector<RecordingBeat> &beats = m_timeline->Beats();
    const double now = videoSecondsNow();
    const auto after = std::upper_bound(beats.begin(), beats.end(), now,
                                        [](double s, const RecordingBeat &beat)
                                        { return s < beat.seconds; });
    if (after == beats.begin())
    {
      // Before its first anchor (just opened, say): there, waiting.
      const RecordingBeat &first = beats.front();
      span = {{"from", first.writtenTick},
              {"to", first.writtenTick + first.beatTicks},
              {"fraction", 0.0},
              {"msLeft", 0.0}};
    }
    else
    {
      const RecordingBeat &beat = *(after - 1);
      // To the next anchor, as far on as it is played: over a repeat's end,
      // as written, it reaches past it; before a skip, or at the end, a beat.
      const bool onToNext = after != beats.end() && !after->afterSkip;
      const int length = onToNext ? after->tick - beat.tick : beat.beatTicks;
      const double seconds =
          after != beats.end() ? after->seconds - beat.seconds : 0.0;
      const double fraction =
          seconds > 0.0 ? std::clamp((now - beat.seconds) / seconds, 0.0, 1.0)
                        : 0.0;
      const double msLeft =
          m_videoClock.Playing() && seconds > 0.0
              ? (after->seconds - now) / m_videoClock.Rate() * 1000.0
              : 0.0;
      span = {{"from", beat.writtenTick},
              {"to", beat.writtenTick + length},
              {"fraction", fraction},
              {"msLeft", std::max(0.0, msLeft)}};
    }
    if (m_videoClock.Playing() && after != beats.end())
      m_followSpanTimer.start(static_cast<int>(
          std::ceil((after->seconds - now) / m_videoClock.Rate() * 1000.0)));
  }
  // Told on each report of the video's clock too: the position between
  // anchors is told afresh, not only the anchors.
  m_followSpan = span;
  emit followSpanChanged();
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
    if (m_videoClock.Playing() && active + 1 < static_cast<int>(points.size()))
      m_activeSyncBeatTimer.start(static_cast<int>(std::ceil(
          (points[active + 1].seconds - now) / m_videoClock.Rate() * 1000.0)));
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
    if (const QUrl url = youTubeFrame(m_recordings[m_current].url);
        !url.isEmpty())
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
