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

#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "playback/iplaybackcontroller.h"
#include <QObject>

namespace dgk
{
class PlaybackButtonModel : public QObject, public muse::async::Asyncable
{
  Q_OBJECT

  Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
  Q_PROPERTY(bool isPlayAllowed READ isPlayAllowed NOTIFY isPlayAllowedChanged)

  INJECT(mu::playback::IPlaybackController, playbackController);
  INJECT(muse::actions::IActionsDispatcher, dispatcher);

public:
  explicit PlaybackButtonModel(QObject *parent = nullptr);

  bool isPlaying() const;
  bool isPlayAllowed() const;

  Q_INVOKABLE void load();
  Q_INVOKABLE void togglePlay();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void rewind();
  Q_INVOKABLE void backStep();
  Q_INVOKABLE void forwardStep();

signals:
  void isPlayingChanged();
  void isPlayAllowedChanged();
};
} // namespace dgk
