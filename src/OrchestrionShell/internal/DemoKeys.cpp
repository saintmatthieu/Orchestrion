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
#include "DemoKeys.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QWindow>
#include <utility>

namespace dgk
{
namespace
{
// The row's index of a digit key: 1 to 9, then 0. -1 for anything else.
int keyIndex(QChar key)
{
  if (key < '0' || key > '9')
    return -1;
  return key == '0' ? 9 : key.digitValue() - 1;
}

// The automatic player's keys, two per hand
constexpr int leftKeys[] = {1, 2};  // 2 and 3
constexpr int rightKeys[] = {7, 8}; // 8 and 9
} // namespace

DemoKeys::DemoKeys(QObject *parent) : QObject(parent) {}

void DemoKeys::init()
{
  computerKeyboard()->keyPressed().onReceive(
      this, [this](char key) { setKeyDown(keyIndex(QChar(key)), true); });
  computerKeyboard()->keyReleased().onReceive(
      this, [this](char key) { setKeyDown(keyIndex(QChar(key)), false); });

  followSequencer();
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { followSequencer(); });
}

void DemoKeys::followSequencer()
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  sequencer->HandNoteEvents().onReceive(
      this,
      [this](const AutoPlayEvent &event)
      {
        // The performer's gestures show as their keys already.
        const auto player = orchestrion()->player();
        if (!player || !player->IsPlaying())
          return;
        int &held = event.isLeftHand ? m_leftAutoKey : m_rightAutoKey;
        if (held >= 0)
          setKeyDown(std::exchange(held, -1), false);
        if (event.type != NoteEventType::noteOn)
          return;
        bool &nextIsSecond =
            event.isLeftHand ? m_leftNextIsSecond : m_rightNextIsSecond;
        held = (event.isLeftHand ? leftKeys : rightKeys)[nextIsSecond ? 1 : 0];
        nextIsSecond = !nextIsSecond;
        setKeyDown(held, true);
      },
      muse::async::Asyncable::Mode::SetReplace);
}

void DemoKeys::setShown(bool shown)
{
  if (shown == m_shown)
    return;
  m_shown = shown;
  emit shownChanged();
}

void DemoKeys::setCaption(const QString &caption)
{
  if (caption == m_caption)
    return;
  m_caption = caption;
  emit captionChanged();
}

bool DemoKeys::press(QChar key) { return sendKey(QEvent::KeyPress, key); }

bool DemoKeys::release(QChar key) { return sendKey(QEvent::KeyRelease, key); }

bool DemoKeys::sendKey(QEvent::Type type, QChar key)
{
  if (keyIndex(key) < 0)
    return false;
  QKeyEvent event(type, Qt::Key_0 + key.digitValue(), Qt::NoModifier,
                  QString(key));
  QCoreApplication::sendEvent(mainWindow()->qWindow(), &event);
  return true;
}

void DemoKeys::setKeyDown(int index, bool down)
{
  if (index < 0)
    return;
  const int keys =
      down ? m_pressedKeys | (1 << index) : m_pressedKeys & ~(1 << index);
  if (keys == m_pressedKeys)
    return;
  m_pressedKeys = keys;
  emit pressedKeysChanged();
}

bool DemoKeys::shown() const { return m_shown; }

QString DemoKeys::caption() const { return m_caption; }

int DemoKeys::pressedKeys() const { return m_pressedKeys; }
} // namespace dgk
