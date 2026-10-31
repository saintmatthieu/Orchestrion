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

#include "GestureControllers/IComputerKeyboard.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"

#include <async/asyncable.h>
#include <ui/imainwindow.h>

#include <QObject>
#include <QString>

namespace dgk
{
/**
 * What demos show of the computer keyboard: the number keys 1 to 0, going
 * down as they are pressed, and a caption. DemoKeysOverlay.qml draws them.
 *
 * The keys follow the computer keyboard itself, whoever presses it: a demo
 * script (press() and release() send the window real key events), or the
 * user. The automatic player presses no keys: while it plays, each hand's
 * gestures show on two keys it alternates between, 2 and 3 for the left hand,
 * 8 and 9 for the right.
 */
class DemoKeys : public QObject,
                 public dgk::Injectable,
                 public muse::async::Asyncable
{
  Q_OBJECT

  Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)
  Q_PROPERTY(QString caption READ caption NOTIFY captionChanged)
  /** Bit i: the i-th key of 1, 2, ..., 9, 0 is down. */
  Q_PROPERTY(int pressedKeys READ pressedKeys NOTIFY pressedKeysChanged)

  dgk::Inject<IComputerKeyboard> computerKeyboard{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<muse::ui::IMainWindow> mainWindow{this};

public:
  explicit DemoKeys(QObject *parent = nullptr);

  void init();

  void setShown(bool shown);
  /** Empty hides it. */
  void setCaption(const QString &caption);

  /** `key` is '0' to '9'; false if it isn't. */
  bool press(QChar key);
  bool release(QChar key);

  /**
   * Releases the keys still down, and hides the keys and the caption: what a
   * stopped demo leaves.
   */
  void reset();

  bool shown() const;
  QString caption() const;
  int pressedKeys() const;

signals:
  void shownChanged();
  void captionChanged();
  void pressedKeysChanged();

private:
  void followSequencer();
  bool sendKey(QEvent::Type type, QChar key);
  void setKeyDown(int index, bool down);

  bool m_shown = false;
  QString m_caption;
  int m_pressedKeys = 0;
  // The automatic player's key for each hand's next gesture, and the one it
  // holds down (-1: none)
  bool m_leftNextIsSecond = false;
  bool m_rightNextIsSecond = false;
  int m_leftAutoKey = -1;
  int m_rightAutoKey = -1;
};
} // namespace dgk
