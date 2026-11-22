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
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "global/iglobalconfiguration.h"
#include "interactive/iplatforminteractive.h"
#include "modularity/ioc.h"
#include "project/iprojectsession.h"

#include <QObject>
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
   * Syncing the video being watched with the score: tapping its beats as it
   * plays (see startSync).
   */
  Q_PROPERTY(bool syncing READ syncing NOTIFY syncChanged)
  /** The beat to tap next, e.g. "bar 12, beat 3". */
  Q_PROPERTY(QString syncNext READ syncNext NOTIFY syncChanged)
  /** How many beats are tapped. */
  Q_PROPERTY(int syncCount READ syncCount NOTIFY syncChanged)

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
   * Starts syncing the video being watched, or the first, with the score: as
   * it plays, each beat is tapped (tapBeat), from where its sync left off.
   * Clicking a bar makes it the next to tap: ahead, the video skips to it (a
   * repeat it doesn't play); behind, the beats from it are tapped again, the
   * video going back to just before it. Each tap is saved at once.
   */
  Q_INVOKABLE void startSync();
  Q_INVOKABLE void tapBeat();
  Q_INVOKABLE void undoBeat();
  /** Ends syncing: the score then follows the video by what was tapped. */
  Q_INVOKABLE void finishSync();

  QStringList performers() const;
  bool offerShown() const;
  int currentIndex() const;
  QUrl playerUrl() const;
  QUrl thumbnailUrl() const;
  bool syncing() const;
  QString syncNext() const;
  int syncCount() const;

signals:
  void recordingsChanged();
  void offerShownChanged();
  void currentChanged();
  void thumbnailChanged();
  void syncChanged();
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
  void saveSync();
  /** The bar clicked while syncing, as written, is the next to tap. */
  void syncFrom(int writtenTick);
  void advanceSync();

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
  // Syncing: the beats tapped, the bars to tap them in, the next to tap.
  bool m_syncing = false;
  RecordingSync m_sync;
  std::vector<PlayedBar> m_bars;
  int m_nextBar = 0;
  int m_nextBeat = 1;
  int m_current = -1;
  bool m_offerClosed = false;
  bool m_offerShown = false;
  PerformanceEndWatcher m_end;
};
} // namespace dgk
