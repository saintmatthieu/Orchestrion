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

#include "ISynthesizerManager.h"
#include "IOrchestrionSynthesisConfiguration.h"
#include <async/asyncable.h>
#include <modularity/ioc.h>

#include <atomic>
#include <memory>

namespace dgk
{
class OrchestrionSynthesisConfiguration
    : public IOrchestrionSynthesisConfiguration,
      public muse::async::Asyncable,
      public muse::Injectable
{
public:
  void init();
  void postInit();

private:
  ReverbPreset reverbPreset() const override;
  void setReverbPreset(ReverbPreset) override;
  muse::async::Notification reverbPresetChanged() const override;
  std::shared_ptr<const std::atomic<int>>
  reverbPresetForAudioThread() const override;

  muse::Inject<ISynthesizerManager> synthManager;

  const std::shared_ptr<std::atomic<int>> m_reverbPreset =
      std::make_shared<std::atomic<int>>(
          static_cast<int>(ReverbPreset::Hall));
  muse::async::Notification m_reverbPresetChanged;
};
} // namespace dgk
