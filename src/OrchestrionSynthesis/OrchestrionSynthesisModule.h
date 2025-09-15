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

#include "audio/worker/isynthresolver.h"
#include "global/modularity/imodulesetup.h"

namespace dgk
{
class SynthesizerConnector;
class SynthesizerManager;
class OrchestrionSynthesisConfiguration;

class OrchestrionSynthesisModule : public muse::modularity::IModuleSetup
{
public:
  OrchestrionSynthesisModule();

private:
  std::string moduleName() const override;
  void registerExports() override;
  void onInit(const muse::IApplication::RunMode &mode) override;
  void onAllInited(const muse::IApplication::RunMode &mode) override;
  void onDelayedInit() override;

  const std::shared_ptr<SynthesizerConnector> m_synthesizerConnector;
  const std::shared_ptr<SynthesizerManager> m_synthesizerManager;
  const std::unique_ptr<OrchestrionSynthesisConfiguration> m_configuration;
};

} // namespace dgk