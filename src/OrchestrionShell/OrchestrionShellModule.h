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

#include <memory>

#include "modularity/imodulesetup.h"

namespace dgk
{
class ControllerMenuManager;
class OrchestrionUiActions;
class PlaybackDeviceMenuManager;
class SynthesizerMenuManager;
class OrchestrionEventProcessor;
class OrchestrionActionController;

class OrchestrionShellModule : public muse::modularity::IModuleSetup
{
public:
  OrchestrionShellModule();

  std::string moduleName() const override;
  void registerExports() override;
  void resolveImports() override;

  void registerUiTypes() override;

  void onPreInit(const muse::IApplication::RunMode &mode) override;
  void onInit(const muse::IApplication::RunMode &mode) override;
  void onAllInited(const muse::IApplication::RunMode &mode) override;

private:
  const std::shared_ptr<ControllerMenuManager> m_midiControllerMenuManager;
  const std::shared_ptr<SynthesizerMenuManager> m_midiSynthesizerMenuManager;
  const std::shared_ptr<PlaybackDeviceMenuManager> m_playbackDeviceMenuManager;
  const std::shared_ptr<OrchestrionEventProcessor> m_orchestrionEventProcessor;
  const std::shared_ptr<OrchestrionUiActions> m_orchestrionUiActions;
  const std::shared_ptr<OrchestrionActionController> m_orchestrionActionController;
};
} // namespace dgk
