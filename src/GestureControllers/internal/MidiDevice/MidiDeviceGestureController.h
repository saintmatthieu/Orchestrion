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

#include "IGestureController.h"

#include "async/asyncable.h"
#include "midi/imidiinport.h"
#include "modularity/ioc.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
class MidiDeviceGestureController : public IGestureController,
                                    public dgk::Injectable,
                                    public muse::async::Asyncable
{
public:
  MidiDeviceGestureController();

private:
  void onMidiEventReceived(const muse::midi::Event &event);
  muse::async::Channel<int, std::optional<float>> noteOn() const override;
  muse::async::Channel<int> noteOff() const override;
  muse::async::Channel<bool> sustainPedal() const override;

  dgk::Inject<muse::midi::IMidiInPort> midiInPort{this};
  muse::async::Channel<int, std::optional<float>> m_noteOn;
  muse::async::Channel<int> m_noteOff;
  muse::async::Channel<bool> m_sustainPedal;
  /**
   * Where the sustain pedal is, empty until it has first gone down: a
   * keyboard may send "pedal up" on its own (a controller reset), which must
   * not pass for a pedal being there.
   */
  std::optional<bool> m_sustainDown;
};

} // namespace dgk