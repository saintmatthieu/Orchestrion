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
#include "TourRhythmDemoModel.h"

#include "audio/iaudiooutput.h"

namespace dgk
{
TourRhythmDemoModel::TourRhythmDemoModel(QObject *parent) : QObject(parent) {}

void TourRhythmDemoModel::init()
{
  subscribeToSequencer();
  orchestrion()->sequencerChanged().onNotify(this, [this]
                                             { subscribeToSequencer(); });

  // Loop: when the score reaches the end while the page is showing, start
  // over. (Our own stop() flips m_active off before dispatching "stop".)
  playbackController()->isPlayingChanged().onNotify(
      this,
      [this]
      {
        if (playbackController()->isPlaying())
          return;
        m_alternator.reset();
        updateKeys();
        if (m_active)
          startPlayback();
      });

  // The score may still be loading when the page appears.
  playbackController()->isPlayAllowedChanged().onNotify(
      this,
      [this]
      {
        if (m_active && playbackController()->isPlayAllowed() &&
            !playbackController()->isPlaying())
          startPlayback();
      });
}

void TourRhythmDemoModel::subscribeToSequencer()
{
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  sequencer->HandNoteEvents().onReceive(this,
                                        [this](const AutoPlayEvent &event)
                                        {
                                          if (!m_active)
                                            return;
                                          m_alternator.onEvent(event);
                                          updateKeys();
                                        });
}

void TourRhythmDemoModel::start()
{
  if (m_active)
    return;
  m_active = true;
  m_alternator.reset();
  updateKeys();
  if (playbackController()->isPlayAllowed() &&
      !playbackController()->isPlaying())
    startPlayback();
}

void TourRhythmDemoModel::startPlayback()
{
  if (!playbackController()->isPlayAllowed())
    return;
  dispatcher()->dispatch("rewind");
  dispatcher()->dispatch("play");
}

void TourRhythmDemoModel::stop()
{
  if (!m_active)
    return;
  m_active = false;
  if (playbackController()->isPlaying())
    dispatcher()->dispatch("stop");
  // Leave the score ready to be played from the top.
  dispatcher()->dispatch("rewind");
  m_alternator.reset();
  updateKeys();
  if (m_muted)
  {
    m_muted = false;
    emit mutedChanged();
    setMasterMuted(false);
  }
}

void TourRhythmDemoModel::toggleMuted()
{
  m_muted = !m_muted;
  emit mutedChanged();
  setMasterMuted(m_muted);
}

void TourRhythmDemoModel::setMasterMuted(bool muted)
{
  playback()->audioOutput()->masterOutputParams().onResolve(
      this,
      [this, muted](muse::audio::AudioOutputParams params)
      {
        params.muted = muted;
        playback()->audioOutput()->setMasterOutputParams(params);
      });
}

void TourRhythmDemoModel::updateKeys()
{
  if (m_leftPressedKey != m_alternator.leftKey())
  {
    m_leftPressedKey = m_alternator.leftKey();
    emit leftPressedKeyChanged();
  }
  if (m_rightPressedKey != m_alternator.rightKey())
  {
    m_rightPressedKey = m_alternator.rightKey();
    emit rightPressedKeyChanged();
  }
}

int TourRhythmDemoModel::leftPressedKey() const { return m_leftPressedKey; }
int TourRhythmDemoModel::rightPressedKey() const { return m_rightPressedKey; }
bool TourRhythmDemoModel::muted() const { return m_muted; }
} // namespace dgk
