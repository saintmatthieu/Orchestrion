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

#include "framework/global/async/notification.h"
#include "framework/global/modularity/ioc.h"

#include <atomic>
#include <memory>

namespace dgk
{
// Standard reverb sizes, ordered from driest to wettest. The enum values are
// persisted to settings, so don't renumber them.
enum class ReverbPreset
{
  Off = 0,
  Room = 1,
  Hall = 2,
  Cathedral = 3,
};

class IOrchestrionSynthesisConfiguration : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionSynthesisConfiguration);

public:
  virtual ~IOrchestrionSynthesisConfiguration() = default;

  virtual ReverbPreset reverbPreset() const = 0;
  virtual void setReverbPreset(ReverbPreset) = 0;
  // For the UI; fires on the thread that changed the setting (the main thread).
  virtual muse::async::Notification reverbPresetChanged() const = 0;

  // A lock-free view of the current preset for the audio thread to poll. The
  // returned pointer stays valid for the lifetime of the configuration.
  virtual std::shared_ptr<const std::atomic<int>>
  reverbPresetForAudioThread() const = 0;
};
} // namespace dgk
