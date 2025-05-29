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
#include "OrchestrionSynthesisModule.h"
#include "internal/OrchestrionSynthesisConfiguration.h"
#include "internal/SynthesizerConnector.h"
#include "internal/SynthesizerManager.h"
#include "internal/TrackChannelMapper.h"

namespace dgk
{
OrchestrionSynthesisModule::OrchestrionSynthesisModule()
    : m_synthesizerConnector{std::make_shared<SynthesizerConnector>()},
      m_synthesizerManager{std::make_shared<SynthesizerManager>()},
      m_configuration{std::make_unique<OrchestrionSynthesisConfiguration>()}
{
}

std::string OrchestrionSynthesisModule::moduleName() const
{
  return "OrchestrionSynthesis";
}

void OrchestrionSynthesisModule::registerExports()
{
  ioc()->registerExport<ISynthesizerConnector>(moduleName(),
                                               m_synthesizerConnector);
  ioc()->registerExport<ITrackChannelMapper>(moduleName(),
                                             new TrackChannelMapper);
  ioc()->registerExport<ISynthesizerManager>(moduleName(),
                                             m_synthesizerManager);
}

void OrchestrionSynthesisModule::onInit(const muse::IApplication::RunMode &)
{
  m_configuration->init();
  m_synthesizerManager->init();
}

void OrchestrionSynthesisModule::onAllInited(
    const muse::IApplication::RunMode &)
{
  m_synthesizerManager->onAllInited();
  m_synthesizerConnector->onAllInited();
}

void OrchestrionSynthesisModule::onDelayedInit()
{
  m_configuration->postInit();
}
} // namespace dgk