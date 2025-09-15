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

#include "global/async/asyncable.h"
#include "midi/imidiinport.h"
#include "global/modularity/ioc.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace dgk
{
class MidiDeviceGestureController : public IGestureController,
                                    public muse::Injectable,
                                    public muse::async::Asyncable
{
public:
  MidiDeviceGestureController();

  static bool isFunctional() { return true; }

private:
  void onMidiEventReceived(const muse::midi::Event &event);
  muse::async::Channel<int, std::optional<float>> noteOn() const override;
  muse::async::Channel<int> noteOff() const override;

  muse::Inject<muse::midi::IMidiInPort> midiInPort;
  muse::async::Channel<int, std::optional<float>> m_noteOn;
  muse::async::Channel<int> m_noteOff;
};

} // namespace dgk