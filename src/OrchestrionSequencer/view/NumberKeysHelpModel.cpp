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
#include "NumberKeysHelpModel.h"

namespace dgk
{
namespace
{
constexpr int leftKeyA = 2; // left middle finger
constexpr int leftKeyB = 3; // left index finger
constexpr int rightKeyA = 8; // right index finger
constexpr int rightKeyB = 9; // right middle finger

// Dispatched by the "Help" menu to (re)open the tip. Must match the UiAction
// registered in OrchestrionUiActions and the menu item in OrchestrionMenuModel.
constexpr auto showHelpActionCode = "orchestrion-help-number-keys";
} // namespace

NumberKeysHelpModel::NumberKeysHelpModel(QObject *parent) : QObject(parent) {}

void NumberKeysHelpModel::init()
{
  m_dismissed = configuration()->keyboardHelpDismissed();

  updateNoMidiConnected();
  midiDeviceService()->availableDevicesChanged().onNotify(
      this, [this] { updateNoMidiConnected(); });
  midiDeviceService()->selectedDeviceChanged().onNotify(
      this, [this] { updateNoMidiConnected(); });

  subscribeToSequencer();
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { subscribeToSequencer(); });

  playbackController()->isPlayingChanged().onNotify(
      this,
      [this]
      {
        // The score auto-stops at the end (or the user may stop): end the demo
        // so the overlay hides and the tooltip can return.
        if (m_demoActive && !playbackController()->isPlaying())
          stop();
      });

  // The Help menu replays the demo directly.
  dispatcher()->reg(this, showHelpActionCode, [this] { showMe(); });
}

void NumberKeysHelpModel::subscribeToSequencer()
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  sequencer->HandNoteEvents().onReceive(
      this, [this](const AutoPlayEvent &event) { onHandNoteEvent(event); });
}

void NumberKeysHelpModel::onHandNoteEvent(const AutoPlayEvent &event)
{
  if (event.isLeftHand)
  {
    if (event.type == NoteEventType::noteOn)
    {
      setLeftPressedKey(m_leftNextIsSecond ? leftKeyB : leftKeyA);
      m_leftNextIsSecond = !m_leftNextIsSecond;
    }
    else
      setLeftPressedKey(0);
  }
  else
  {
    if (event.type == NoteEventType::noteOn)
    {
      setRightPressedKey(m_rightNextIsSecond ? rightKeyB : rightKeyA);
      m_rightNextIsSecond = !m_rightNextIsSecond;
    }
    else
      setRightPressedKey(0);
  }
}

void NumberKeysHelpModel::showMe()
{
  if (!playbackController()->isPlayAllowed())
    return;

  m_leftNextIsSecond = false;
  m_rightNextIsSecond = false;
  setLeftPressedKey(0);
  setRightPressedKey(0);
  setDemoActive(true);

  if (!playbackController()->isPlaying())
  {
    dispatcher()->dispatch("rewind");
    dispatcher()->dispatch("play");
  }
}

void NumberKeysHelpModel::stop()
{
  if (playbackController()->isPlaying())
    dispatcher()->dispatch("stop");
  setLeftPressedKey(0);
  setRightPressedKey(0);
  setDemoActive(false);
}

void NumberKeysHelpModel::dismiss()
{
  if (!m_dismissed)
  {
    m_dismissed = true;
    configuration()->setKeyboardHelpDismissed(true);
  }
  updateTooltipVisible();
}

void NumberKeysHelpModel::updateNoMidiConnected()
{
  bool connected = false;
  if (const auto device = midiDeviceService()->selectedDevice())
    connected = midiDeviceService()->isAvailable(*device) &&
                !midiDeviceService()->isNoDevice(*device);
  m_noMidiConnected = !connected;
  updateTooltipVisible();
}

void NumberKeysHelpModel::updateTooltipVisible()
{
  const bool visible = !m_demoActive && m_noMidiConnected && !m_dismissed;
  if (m_tooltipVisible == visible)
    return;
  m_tooltipVisible = visible;
  emit tooltipVisibleChanged();
}

void NumberKeysHelpModel::setDemoActive(bool value)
{
  if (m_demoActive == value)
    return;
  m_demoActive = value;
  emit demoActiveChanged();
  updateTooltipVisible();
}

void NumberKeysHelpModel::setLeftPressedKey(int value)
{
  if (m_leftPressedKey == value)
    return;
  m_leftPressedKey = value;
  emit leftPressedKeyChanged();
}

void NumberKeysHelpModel::setRightPressedKey(int value)
{
  if (m_rightPressedKey == value)
    return;
  m_rightPressedKey = value;
  emit rightPressedKeyChanged();
}

bool NumberKeysHelpModel::tooltipVisible() const { return m_tooltipVisible; }
bool NumberKeysHelpModel::demoActive() const { return m_demoActive; }
int NumberKeysHelpModel::leftPressedKey() const { return m_leftPressedKey; }
int NumberKeysHelpModel::rightPressedKey() const { return m_rightPressedKey; }
} // namespace dgk
