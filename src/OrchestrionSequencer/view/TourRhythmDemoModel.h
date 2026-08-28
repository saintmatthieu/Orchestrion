/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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

#include "IOrchestrion.h"
#include "NumberKeysAlternator.h"

#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "audio/iplayback.h"
#include "modularity/ioc.h"
#include "playback/iplaybackcontroller.h"
#include <QObject>

namespace dgk
{
//! Drives the welcome tour's "tap the rhythm" page: auto-plays the score that
//! is open behind the tour (looping while the page is showing) and reports
//! which number keys the hands press, for NumberKeysAnimation to mirror.
//! The master output can be muted while the demo keeps running.
class TourRhythmDemoModel : public QObject,
                            public muse::async::Asyncable,
                            public muse::Injectable
{
  Q_OBJECT

  Q_PROPERTY(
      int leftPressedKey READ leftPressedKey NOTIFY leftPressedKeyChanged)
  Q_PROPERTY(
      int rightPressedKey READ rightPressedKey NOTIFY rightPressedKeyChanged)
  Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)

  muse::Inject<IOrchestrion> orchestrion;
  //! Only for isPlayAllowed(): whether a score is loaded and playable. The
  //! playing state itself comes from Orchestrion's own player.
  muse::Inject<mu::playback::IPlaybackController> playbackController;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;
  muse::Inject<muse::audio::IPlayback> playback;

public:
  explicit TourRhythmDemoModel(QObject *parent = nullptr);

  Q_INVOKABLE void init();
  //! Start auto-playing the open score (or as soon as it is playable).
  Q_INVOKABLE void start();
  //! Stop playback and restore the master output's mute state.
  Q_INVOKABLE void stop();
  Q_INVOKABLE void toggleMuted();

  int leftPressedKey() const;
  int rightPressedKey() const;
  bool muted() const;

signals:
  void leftPressedKeyChanged();
  void rightPressedKeyChanged();
  void mutedChanged();

private:
  void subscribeToSequencer();
  bool isPlaying() const;
  void startPlayback();
  void setMasterMuted(bool);
  void updateKeys();

  NumberKeysAlternator m_alternator;
  bool m_active = false;
  bool m_muted = false;
  int m_leftPressedKey = 0;
  int m_rightPressedKey = 0;
};
} // namespace dgk
