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
#include "MuseScoreShell/OrchestrionActionIds.h"

namespace dgk
{
namespace
{
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
  orchestrion()->sequencerChanged().onNotify(this, [this]
                                             { subscribeToSequencer(); });

  // The Help menu replays the demo directly.
  dispatcher()->reg(this, showHelpActionCode, [this] { showMe(); });
}

void NumberKeysHelpModel::subscribeToSequencer()
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  sequencer->HandNoteEvents().onReceive(this, [this](const AutoPlayEvent &event)
                                        { onHandNoteEvent(event); });

  // The playback auto-stops at the end (or the user may stop): end the demo
  // so the overlay hides and the tooltip can return. (The player lives and
  // dies with the sequencer, hence the subscription here.)
  orchestrion()->player()->PlayingChanged().onNotify(this,
                                                     [this]
                                                     {
                                                       if (m_demoActive &&
                                                           !isPlaying())
                                                         stop();
                                                     });
}

bool NumberKeysHelpModel::isPlaying() const
{
  return orchestrion()->player()->IsPlaying();
}

void NumberKeysHelpModel::onHandNoteEvent(const AutoPlayEvent &event)
{
  m_alternator.onEvent(event);
  setLeftPressedKey(m_alternator.leftKey());
  setRightPressedKey(m_alternator.rightKey());
}

void NumberKeysHelpModel::showMe()
{
  if (!playbackController()->isPlayAllowed())
    return;

  m_alternator.reset();
  setLeftPressedKey(0);
  setRightPressedKey(0);
  setDemoActive(true);

  if (!isPlaying())
  {
    dispatcher()->dispatch("rewind");
    dispatcher()->dispatch(actionIds::playbackToggle);
  }
}

void NumberKeysHelpModel::stop()
{
  if (isPlaying())
    dispatcher()->dispatch(actionIds::playbackStop);
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
  m_noMidiConnected = !midiDeviceService()->realDeviceConnected();
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
