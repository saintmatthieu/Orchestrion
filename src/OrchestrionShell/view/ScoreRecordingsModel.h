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
#include "async/asyncable.h"
#include "interactive/iplatforminteractive.h"
#include "modularity/ioc.h"
#include "project/iprojectsession.h"

#include <QObject>
#include <QUrl>
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
                             public muse::async::Asyncable
{
  Q_OBJECT

  dgk::Inject<mu::project::IProjectSession> projectSession{this};
  dgk::Inject<muse::IPlatformInteractive> platformInteractive{this};
  dgk::Inject<IRollsLibrary> rollsLibrary{this};
  dgk::Inject<IOrchestrion> orchestrion{this};

  /** The performers' names, best first. */
  Q_PROPERTY(QStringList performers READ performers NOTIFY recordingsChanged)
  Q_PROPERTY(bool offerShown READ offerShown NOTIFY offerShownChanged)
  /** The one being watched; -1 if none. */
  Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentChanged)
  /** The page that plays it. */
  Q_PROPERTY(QUrl playerUrl READ playerUrl NOTIFY currentChanged)

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

  QStringList performers() const;
  bool offerShown() const;
  int currentIndex() const;
  QUrl playerUrl() const;

signals:
  void recordingsChanged();
  void offerShownChanged();
  void currentChanged();
  /** The performer is playing: the video isn't to play over them. */
  void pauseRequested();

private:
  void update();
  void setOfferShown(bool);

  std::vector<ScoreRecording> m_recordings;
  int m_current = -1;
  bool m_offerClosed = false;
  bool m_offerShown = false;
  PerformanceEndWatcher m_end;
};
} // namespace dgk
