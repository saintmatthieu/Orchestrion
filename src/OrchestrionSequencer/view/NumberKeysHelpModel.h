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

#include "ExternalDevices/IMidiDeviceService.h"
#include "IOrchestrion.h"
#include "IOrchestrionSequencerConfiguration.h"

#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "playback/iplaybackcontroller.h"
#include <QObject>

namespace dgk
{
//! Beginner help: when no MIDI controller is connected, the view shows a
//! "Use your number keys to play." tooltip. It auto-shows only until the user
//! closes it (the dismissal is persisted). The Help menu replays the demo
//! directly. Clicking "Show me!" (or the Help menu) starts automatic playback
//! and this model reports which number key each hand is "pressing", so the QML
//! keyboard animation can follow the music.
class NumberKeysHelpModel : public QObject,
                           public muse::async::Asyncable,
                           public muse::actions::Actionable,
                           public muse::Injectable
{
  Q_OBJECT

  Q_PROPERTY(bool tooltipVisible READ tooltipVisible NOTIFY tooltipVisibleChanged)
  Q_PROPERTY(bool demoActive READ demoActive NOTIFY demoActiveChanged)
  Q_PROPERTY(int leftPressedKey READ leftPressedKey NOTIFY leftPressedKeyChanged)
  Q_PROPERTY(
      int rightPressedKey READ rightPressedKey NOTIFY rightPressedKeyChanged)

  muse::Inject<IMidiDeviceService> midiDeviceService;
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<IOrchestrionSequencerConfiguration> configuration;
  muse::Inject<mu::playback::IPlaybackController> playbackController;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;

public:
  explicit NumberKeysHelpModel(QObject *parent = nullptr);

  Q_INVOKABLE void init();
  //! Start the demo: rewind + play the score and animate the keys.
  Q_INVOKABLE void showMe();
  //! Stop the demo and hide the animation overlay.
  Q_INVOKABLE void stop();
  //! Close the tip and remember it (persisted) so it won't auto-show again.
  Q_INVOKABLE void dismiss();

  bool tooltipVisible() const;
  bool demoActive() const;
  int leftPressedKey() const;
  int rightPressedKey() const;

signals:
  void tooltipVisibleChanged();
  void demoActiveChanged();
  void leftPressedKeyChanged();
  void rightPressedKeyChanged();

private:
  void subscribeToSequencer();
  void onHandNoteEvent(const AutoPlayEvent &);
  void updateNoMidiConnected();

  void updateTooltipVisible();
  void setDemoActive(bool);
  void setLeftPressedKey(int);
  void setRightPressedKey(int);

  bool m_noMidiConnected = false;
  bool m_demoActive = false;
  // Persisted: the user has closed the tip, so don't auto-show it.
  bool m_dismissed = false;
  bool m_tooltipVisible = false;
  int m_leftPressedKey = 0;
  int m_rightPressedKey = 0;
  // Alternation state: each hand alternates between its two finger keys
  // (the left/right Key constants in the .cpp).
  bool m_leftNextIsSecond = false;
  bool m_rightNextIsSecond = false;
};
} // namespace dgk
