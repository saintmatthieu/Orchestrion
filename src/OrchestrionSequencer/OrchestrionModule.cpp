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
#include "OrchestrionModule.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "internal/GestureInputConnector.h"
#include "internal/Orchestrion.h"
#include "internal/OrchestrionSequencerConfiguration.h"
#include "view/NumberKeysHelpModel.h"

#include <QQmlEngine>

namespace dgk
{
OrchestrionModule::OrchestrionModule()
    : m_orchestrion(std::make_shared<Orchestrion>()),
      m_gestureInputConnector(std::make_shared<GestureInputConnector>()),
      m_sequencerConfiguration(
          std::make_shared<OrchestrionSequencerConfiguration>())
{
}

std::string OrchestrionModule::moduleName() const { return "Orchestrion"; }

void OrchestrionModule::registerExports()
{
  globalIoc()->registerExport<IOrchestrion>(moduleName(), m_orchestrion);
  globalIoc()->registerExport<IOrchestrionSequencerConfiguration>(
      moduleName(), m_sequencerConfiguration);
}

void OrchestrionModule::registerUiTypes()
{
  qmlRegisterType<NumberKeysHelpModel>("Orchestrion.OrchestrionSequencer", 1, 0,
                                       "NumberKeysHelpModel");
}

muse::modularity::IContextSetup *OrchestrionModule::newContext(
    const muse::modularity::ContextPtr &ctx) const
{
  ModuleContextSetup::Hooks hooks;
  hooks.onInit = [this](const muse::IApplication::RunMode &mode)
  { onContextInit(mode); };
  return new ModuleContextSetup(ctx, std::move(hooks));
}

void OrchestrionModule::onContextInit(
    const muse::IApplication::RunMode &) const
{
  m_orchestrion->init();
  m_gestureInputConnector->init();
  m_sequencerConfiguration->init();
}
} // namespace dgk
