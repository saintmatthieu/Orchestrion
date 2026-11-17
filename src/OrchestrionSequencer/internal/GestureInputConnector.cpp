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
#include "GestureInputConnector.h"
#include "Orchestrion.h"

#include <log.h>

namespace dgk
{
GestureInputConnector::GestureInputConnector(Orchestrion &orchestrion)
    : m_orchestrion{orchestrion}
{
}

GestureInputConnector::~GestureInputConnector()
{
  // No more gestures posted (one under way is waited for), then none of
  // those posted runs: the token goes on the play thread, where they run.
  // (Never initialized, if the application quit as it started: nothing to
  // let go of, and no play thread.)
  if (!m_gestureInput)
    return;
  m_gestureInput->setReceiver(nullptr);
  m_orchestrion.GetPlayThread().Wait([this] { m_alive.reset(); });
}

void GestureInputConnector::Post(std::function<void()> task)
{
  m_orchestrion.GetPlayThread().Post(
      [alive = std::weak_ptr<int>{m_alive}, task = std::move(task)]
      {
        if (!alive.expired())
          task();
      });
}

void GestureInputConnector::init()
{
  m_gestureInput = gestureInput();
  m_gestureInput->setReceiver(
      [this](const IGestureInput::Gesture &gesture)
      { Post([this, gesture] { onGesture(gesture); }); });
  static_cast<IOrchestrion &>(m_orchestrion)
      .sequencerChanged()
      .onNotify(this, [this] { onSequencerChanged(); });
  onSequencerChanged();
}

void GestureInputConnector::onSequencerChanged()
{
  // The player comes and goes with the sequencer.
  const auto player = static_cast<IOrchestrion &>(m_orchestrion).player();
  player->PlayingChanged().onNotify(
      this,
      [this]
      {
        Post([this, playing = static_cast<IOrchestrion &>(m_orchestrion)
                                  .player()
                                  ->IsPlaying()]
             { setPlayerPlaying(playing); });
      },
      muse::async::Asyncable::Mode::SetReplace);
  Post([this, playing = player->IsPlaying()] { reset(playing); });
}

void GestureInputConnector::onGesture(const IGestureInput::Gesture &gesture)
{
  OrchestrionSequencer *const sequencer = m_orchestrion.PlayThreadSequencer();
  if (sequencer)
    // Before it plays it: told in order with what it causes
    sequencer->ObserveGesture(gesture);
  switch (gesture.kind)
  {
  case IGestureInput::Gesture::Kind::noteOn:
    if (sequencer)
      sequencer->OnInputEvent(NoteEventType::noteOn, gesture.pitch,
                              gesture.velocity);
    // else things don't look ready yet
    break;
  case IGestureInput::Gesture::Kind::noteOff:
    // No sequencer: a key released before a score is loaded (a MIDI
    // keyboard's events arrive from startup on); nothing to release.
    if (sequencer)
      sequencer->OnInputEvent(NoteEventType::noteOff, gesture.pitch, 0.0f);
    break;
  case IGestureInput::Gesture::Kind::sustainPedal:
    m_sustainDown = gesture.down;
    if (sequencer)
      sequencer->OnSustainPedal(gesture.down);
    updateFootPedalling();
    break;
  }
}

void GestureInputConnector::reset(bool playerPlaying)
{
  m_playerPlaying = playerPlaying;
  m_footPedalling = false;
  OrchestrionSequencer *const sequencer = m_orchestrion.PlayThreadSequencer();
  if (sequencer && m_sustainDown)
    sequencer->OnSustainPedal(*m_sustainDown);
  updateFootPedalling();
}

void GestureInputConnector::setPlayerPlaying(bool playing)
{
  m_playerPlaying = playing;
  updateFootPedalling();
}

void GestureInputConnector::updateFootPedalling()
{
  OrchestrionSequencer *const sequencer = m_orchestrion.PlayThreadSequencer();
  const bool footPedalling =
      sequencer && m_sustainDown.has_value() && !m_playerPlaying;
  if (footPedalling == m_footPedalling)
    return;
  m_footPedalling = footPedalling;
  if (!sequencer)
    return;
  if (footPedalling)
    LOGI() << "Following the sustain pedal rather than the score's pedalling";
  sequencer->SetFootPedalling(footPedalling);
}
} // namespace dgk
