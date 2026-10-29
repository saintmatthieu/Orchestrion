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

#include "GestureControllers/IGestureInput.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSequencer/IStaffModes.h"
#include "Take.h"

#include <QElapsedTimer>
#include <async/asyncable.h>
#include <optional>
#include <project/iprojectsession.h>
#include <vector>

namespace dgk
{
/**
 * Records the performer's takes while take recording is on (see
 * IOrchestrionSequencerConfiguration::takeRecordingEnabled): every note,
 * release and sustain pedal from the gesture controllers, and the automatic
 * playback starting and stopping. A jump — a click, a rewind, a step — or a
 * change of the staff modes, the ornament mode or the loop ends the take; the
 * next begins with the next note.
 */
class TakeRecorder : public muse::async::Asyncable, public dgk::Injectable
{
  dgk::Inject<IGestureInput> gestureInput{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration{this};
  dgk::Inject<IStaffModes> staffModes{this};
  dgk::Inject<mu::project::IProjectSession> projectSession{this};

public:
  void init();

  /** The takes recorded since the last Clear(), with a note at least. */
  std::vector<Take> Takes() const;
  void Clear();

  /** Nothing is recorded while paused: while a take is replayed, say. */
  void SetPaused(bool);

private:
  bool Recording() const;
  /** The take under way, if any, ends; the next starts at `tick`. */
  void EndTake(std::optional<int> tick);
  void Add(Take::Event);
  void OnSequencerChanged();
  void OnNotationChanged();
  /** Where the hands strike next: the earliest of the voices' next chords. */
  std::optional<int> NextChordTick() const;
  int BarAt(int tick) const;

  std::vector<Take> m_takes;
  std::optional<Take> m_take;
  /** Where the next take starts, if known otherwise than from the hands. */
  std::optional<int> m_nextStartTick;
  QElapsedTimer m_clock;
  bool m_paused = false;
};
} // namespace dgk
