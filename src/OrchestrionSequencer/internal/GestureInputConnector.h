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

#include "GestureControllers/IGestureInput.h"

#include "async/asyncable.h"
#include "modularity/ioc.h"

#include <functional>
#include <memory>
#include <optional>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
class Orchestrion;

/**
 * Feeds the gesture controllers' note events to the sequencer, and their
 * sustain pedal. There is no telling whether a pedal is plugged into the MIDI
 * keyboard until it is used: the sequencer follows the score's pedal markings
 * until then, and the foot from the first press on, for the rest of the
 * session — except while the automatic player plays, the foot having no part
 * in that.
 *
 * Does so on the play thread (see ../threading.md), where the gestures are
 * posted from whatever thread they come from: the main thread's busyness
 * doesn't delay them. The members below init() are the play thread's.
 */
class GestureInputConnector : public dgk::Injectable,
                              public muse::async::Asyncable
{
  dgk::Inject<IGestureInput> gestureInput{this};

public:
  explicit GestureInputConnector(Orchestrion &);
  ~GestureInputConnector() override;

  /** On the main thread, once the Orchestrion is. */
  void init();

private:
  /** On the main thread. */
  void onSequencerChanged();
  /**
   * Runs the task on the play thread, unless the connector is gone by then.
   * From any thread.
   */
  void Post(std::function<void()>);

  void onGesture(const IGestureInput::Gesture &);
  /** Another sequencer: the foot's state goes to it. */
  void reset(bool playerPlaying);
  void setPlayerPlaying(bool);
  /** Tells the sequencer whether the foot pedals, if that changed. */
  void updateFootPedalling();

  Orchestrion &m_orchestrion;
  /**
   * Kept to let go of it as it is destroyed: by then, quitting, the injected
   * services read null. Its gestures keep coming until it is gone.
   */
  std::shared_ptr<IGestureInput> m_gestureInput;
  /** Let go of on the play thread as it is destroyed (see Post). */
  std::shared_ptr<int> m_alive = std::make_shared<int>(0);
  /** Where the foot has the pedal; empty until a pedal has been used. */
  std::optional<bool> m_sustainDown;
  bool m_footPedalling = false;
  bool m_playerPlaying = false;
};
} // namespace dgk
