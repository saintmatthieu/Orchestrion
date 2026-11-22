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

#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionShell/IRollsLibrary.h"
#include "OrchestrionShell/internal/PerformanceEndWatcher.h"
#include "OrchestrionSequencer/RecordingTimeline.h"
#include "OrchestrionShell/internal/RecordingSync.h"
#include "OrchestrionShell/internal/RecordingSyncSession.h"
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "global/iglobalconfiguration.h"
#include "interactive/iplatforminteractive.h"
#include "modularity/ioc.h"
#include "project/iprojectsession.h"

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QUrl>
#include <chrono>
#include <memory>
#include <vector>

#include "OrchestrionCommon/OrchestrionIoc.h"

namespace dgk
{
/**
 * The videos of the score that is open, the one being watched, in the app if
 * it can be, and the offer to watch them once the performer has played the
 * score to its end: offered at each end until taken up or closed, then not
 * again until a score is opened.
 */
class ScoreRecordingsModel : public QObject,
                             public dgk::Injectable,
                             public muse::async::Asyncable,
                             public muse::actions::Actionable
{
  Q_OBJECT

  dgk::Inject<mu::project::IProjectSession> projectSession{this};
  dgk::Inject<muse::IPlatformInteractive> platformInteractive{this};
  dgk::Inject<IRollsLibrary> rollsLibrary{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};

  /** The performers' names, best first. */
  Q_PROPERTY(QStringList performers READ performers NOTIFY recordingsChanged)
  Q_PROPERTY(bool offerShown READ offerShown NOTIFY offerShownChanged)
  /** The one being watched; -1 if none. */
  Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentChanged)
  /** The page that plays it. */
  Q_PROPERTY(QUrl playerUrl READ playerUrl NOTIFY currentChanged)
  /**
   * A still of the one being watched, else of the first that has one; empty
   * if none has.
   */
  Q_PROPERTY(QUrl thumbnailUrl READ thumbnailUrl NOTIFY thumbnailChanged)
  /**
   * Syncing the video being watched with the score: anchoring it to the
   * score by tapping as it plays (see startSync).
   */
  Q_PROPERTY(bool syncing READ syncing NOTIFY syncChanged)
  /** What to tap next, e.g. "bar 12, beat 3". */
  Q_PROPERTY(QString syncNext READ syncNext NOTIFY syncChanged)
  /** How many anchors there are. */
  Q_PROPERTY(int syncCount READ syncCount NOTIFY syncChanged)
  /** What a tap is for: 0, beats; 1, their subdivisions. */
  Q_PROPERTY(int syncPrecision READ syncPrecision WRITE setSyncPrecision
                 NOTIFY syncChanged)
  /**
   * While syncing, the anchors, in the order played: maps whose "tick" is
   * where the anchor is in the score as written, and "onBeat" whether it is
   * on a beat rather than between. Empty otherwise.
   */
  Q_PROPERTY(QVariantList syncBeats READ syncBeats NOTIFY syncChanged)
  /** The anchors selected, by index. */
  Q_PROPERTY(QVariantList selectedSyncBeats READ selectedSyncBeats NOTIFY
                 syncChanged)
  /** While syncing, the anchor the video is at; -1 if none. */
  Q_PROPERTY(int activeSyncBeat READ activeSyncBeat NOTIFY
                 activeSyncBeatChanged)

public:
  explicit ScoreRecordingsModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();
  /**
   * Shows the video in the app; one that can't play there opens in the
   * browser.
   */
  Q_INVOKABLE void watch(int index);
  Q_INVOKABLE void stopWatching();
  Q_INVOKABLE void watchInBrowser();
  Q_INVOKABLE void closeOffer();
  /**
   * What the player is at: `seconds` into the video, at `rate`, in YouTube's
   * player state `state` (1 is playing). Told as often as asked, a few times
   * a second, so that the score follows the video, if it has a sync file.
   */
  Q_INVOKABLE void reportPlayer(double seconds, double rate, int state);

  /**
   * Starts syncing the video being watched, or the first, with the score
   * (see RecordingSyncSession): as it plays, each beat or subdivision is
   * tapped (tapBeat), from where its sync left off. Clicking a bar makes it
   * the next to tap, the video going back to just before it if it was
   * anchored there already. Each change is saved at once.
   */
  Q_INVOKABLE void startSync();
  Q_INVOKABLE void tapBeat();
  /** Takes the last change back. */
  Q_INVOKABLE void undoSync();
  /** Ends syncing: the score then follows the video by its anchors. */
  Q_INVOKABLE void finishSync();
  /** An anchor: its name ("bar 12, beat 3") and time in the video. */
  Q_INVOKABLE QString syncBeatName(int beat) const;
  Q_INVOKABLE double syncBeatSeconds(int beat) const;
  /** How early and late it may be: just after and before its neighbours. */
  Q_INVOKABLE double syncBeatMinimum(int beat) const;
  Q_INVOKABLE double syncBeatMaximum(int beat) const;
  /**
   * Moves an anchor (seconds into the video, kept between its neighbours),
   * saved; a moment later, the video replays from just before it.
   */
  Q_INVOKABLE void setSyncBeatSeconds(int beat, double seconds);
  /** Selects an anchor; adds it to (or takes it from) the selection if `add`. */
  Q_INVOKABLE void selectSyncBeat(int beat, bool add);
  /**
   * Adds the anchor after the selection (`direction` > 0) or before it; if
   * none is selected, the one the video is at.
   */
  Q_INVOKABLE void extendSyncSelection(int direction);
  /** False if nothing was selected. */
  Q_INVOKABLE bool clearSyncSelection();
  Q_INVOKABLE void deleteSyncSelection();

  QStringList performers() const;
  bool offerShown() const;
  int currentIndex() const;
  QUrl playerUrl() const;
  QUrl thumbnailUrl() const;
  bool syncing() const;
  QString syncNext() const;
  int syncCount() const;
  int syncPrecision() const;
  void setSyncPrecision(int);
  QVariantList syncBeats() const;
  QVariantList selectedSyncBeats() const;
  int activeSyncBeat() const { return m_activeSyncBeat; }

signals:
  void recordingsChanged();
  void offerShownChanged();
  void currentChanged();
  void thumbnailChanged();
  void syncChanged();
  void activeSyncBeatChanged();
  /** The video is to play from `seconds` (into the video). */
  void replayRequested(double seconds);
  /**
   * The performer is playing, or the score is being played: the video isn't
   * to play over them.
   */
  void pauseRequested();
  /** The video is to go to `seconds` (into the video, not the piece). */
  void seekRequested(double seconds);

private:
  void update();
  void setOfferShown(bool);
  /** Subscribes to the player, which is made again with the sequencer. */
  void subscribePlayer();
  /** The sync of the video being watched, if it has one, for the player. */
  void loadSync();
  /** Where the sync of the video being watched is kept; empty if nowhere. */
  QString syncPath() const;
  /** The anchors changed: saved, and told. */
  void syncEdited();
  /** The bar clicked while syncing, as written, is the next to tap. */
  void syncFrom(int writtenTick);
  /** A position, as played: "bar 12, beat 3", "bar 12, beat 3 (2/3)". */
  QString positionName(int position) const;
  /** The piece's seconds the video is at now, by its clock as last told. */
  double videoSecondsNow() const;
  /** Which beat the video is at, and when it reaches the next. */
  void updateActiveSyncBeat();

  std::vector<ScoreRecording> m_recordings;
  std::optional<QString> m_pieceId;
  /** The video being watched: where the piece starts in it, and its sync. */
  double m_startSeconds = 0.0;
  std::shared_ptr<const RecordingTimeline> m_timeline;
  /** Whether the player followed the video, as last told. */
  bool m_following = false;
  /** The player's clock, as last told: seconds into the video. */
  double m_videoSeconds = 0.0;
  std::chrono::steady_clock::time_point m_videoTime;
  double m_videoRate = 1.0;
  bool m_videoPlaying = false;
  /** Syncing: its anchors, what to tap next, ... */
  std::optional<RecordingSyncSession> m_session;
  /** The precision chosen, for each sync of the session. */
  RecordingSyncSession::Precision m_precision =
      RecordingSyncSession::Precision::beats;
  int m_activeSyncBeat = -1;
  QTimer m_activeSyncBeatTimer;
  /** A beat moved: the video replays, once the moving pauses. */
  QTimer m_replayTimer;
  int m_replayBeat = -1;
  int m_current = -1;
  bool m_offerClosed = false;
  bool m_offerShown = false;
  PerformanceEndWatcher m_end;
};
} // namespace dgk
