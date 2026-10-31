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
#include "internal/StaffModes.h"
#include "internal/VisibleScore.h"

#include <QQmlEngine>

namespace dgk
{
OrchestrionModule::OrchestrionModule()
    : m_orchestrion(std::make_shared<Orchestrion>()),
      m_gestureInputConnector(std::make_shared<GestureInputConnector>()),
      m_sequencerConfiguration(
          std::make_shared<OrchestrionSequencerConfiguration>()),
      m_staffModes(std::make_shared<StaffModes>()),
      m_visibleScore(std::make_shared<VisibleScore>())
{
}

std::string OrchestrionModule::moduleName() const { return "Orchestrion"; }

void OrchestrionModule::registerExports()
{
  globalIoc()->registerExport<IOrchestrion>(moduleName(), m_orchestrion);
  globalIoc()->registerExport<IOrchestrionSequencerConfiguration>(
      moduleName(), m_sequencerConfiguration);
  globalIoc()->registerExport<IStaffModes>(moduleName(), m_staffModes);
  globalIoc()->registerExport<IVisibleScore>(moduleName(), m_visibleScore);
}

void OrchestrionModule::registerUiTypes()
{
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
  m_staffModes->init();
}
} // namespace dgk
