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

#include "ExternalDevices/IVirtualMidiKeyboard.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IStaffModes.h"
#include "OrchestrionShell/ITutorials.h"

#include <async/asyncable.h>
#include <midi/imidiinport.h>

#include <QObject>
#include <QVariantList>

namespace dgk
{
/**
 * Backs the keyboard drawn while the virtual MIDI keyboard is the MIDI device
 * in use (VirtualKeyboard.qml): the keys down, struck how hard, whichever
 * device struck them, and the pedal; the drawn keys play the virtual one.
 */
class VirtualKeyboardModel : public QObject,
                             public muse::async::Asyncable,
                             public dgk::Injectable
{
  Q_OBJECT
  Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)
  /** Per MIDI pitch, how hard its key was struck, 0 to 1; 0 when up. */
  Q_PROPERTY(QVariantList velocities READ velocities NOTIFY velocitiesChanged)
  Q_PROPERTY(bool pedalDown READ pedalDown NOTIFY pedalDownChanged)
  /**
   * The keys that play the same thing, in ranges: {from, to (MIDI pitches),
   * role ("left", "right" (the hands' next chords), "notes" (their own
   * pitches))} (see IOrchestrionSequencer::RoleOfKey).
   */
  Q_PROPERTY(QVariantList ranges READ ranges NOTIFY rangesChanged)

  dgk::Inject<IVirtualMidiKeyboard> virtualKeyboard{this};
  dgk::Inject<ITutorials> tutorials{this};
  dgk::Inject<muse::midi::IMidiInPort> midiInPort{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<IStaffModes> staffModes{this};

public:
  explicit VirtualKeyboardModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();
  Q_INVOKABLE void press(int pitch, double velocity);
  Q_INVOKABLE void release(int pitch);

  bool shown() const;
  QVariantList velocities() const;
  bool pedalDown() const;
  QVariantList ranges() const;

signals:
  void shownChanged();
  void velocitiesChanged();
  void pedalDownChanged();
  void rangesChanged();

private:
  void onEvent(const muse::midi::Event &);
  void setVelocity(int pitch, double velocity);
  void updateRanges();

  QVariantList m_velocities;
  bool m_pedalDown = false;
  QVariantList m_ranges;
};
} // namespace dgk
