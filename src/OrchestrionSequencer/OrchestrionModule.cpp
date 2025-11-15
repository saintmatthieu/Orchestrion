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
#include "internal/GestureControllerConfigurator.h"
#include "internal/Orchestrion.h"
#include "internal/OrchestrionSequencerConfiguration.h"
#include "view/GestureControllerSelectionModel.h"
#include "view/MidiDeviceActivityPopupModel.h"

#include "ui/iuiactionsregister.h"
#include <QQmlEngine>

namespace dgk
{
OrchestrionModule::OrchestrionModule()
    : m_orchestrion(std::make_shared<Orchestrion>()),
      m_midiControllerConfigurator(
          std::make_shared<GestureControllerConfigurator>()),
      m_sequencerConfiguration(
          std::make_shared<OrchestrionSequencerConfiguration>())
{
}

std::string OrchestrionModule::moduleName() const { return "Orchestrion"; }

void OrchestrionModule::registerExports()
{
  ioc()->registerExport<IOrchestrion>(moduleName(), m_orchestrion);
  ioc()->registerExport<IGestureControllerConfigurator>(
      moduleName(), m_midiControllerConfigurator);
  ioc()->registerExport<IOrchestrionSequencerConfiguration>(
      moduleName(), m_sequencerConfiguration);
}

void OrchestrionModule::registerUiTypes()
{
  qmlRegisterType<GestureControllerSelectionModel>(
      "Orchestrion.OrchestrionSequencer", 1, 0,
      "GestureControllerSelectionModel");
  qmlRegisterType<MidiDeviceActivityPopupModel>(
      "Orchestrion.OrchestrionSequencer", 1, 0, "MidiDeviceActivityPopupModel");
}

void OrchestrionModule::onInit(const muse::IApplication::RunMode &)
{
  m_orchestrion->init();
  m_midiControllerConfigurator->init();
  m_sequencerConfiguration->init();
}
} // namespace dgk