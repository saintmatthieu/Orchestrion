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
#include "internal/ComputerKeyboard.h"
#include "internal/ComputerKeyboardMidiController.h"
#include "internal/Orchestrion.h"
#include "internal/OrchestrionSequencerActionController.h"
#include "internal/OrchestrionSequencerUiActions.h"
#include <ui/iuiactionsregister.h>

namespace dgk
{
OrchestrionModule::OrchestrionModule()
    : m_orchestrion(std::make_shared<Orchestrion>()),
      m_actionController(
          std::make_shared<OrchestrionSequencerActionController>()),
      m_uiActions(std::make_shared<OrchestrionSequencerUiActions>()),
      m_keyboard(std::make_shared<ComputerKeyboard>()),
      m_keyboardController(std::make_shared<ComputerKeyboardMidiController>())
{
}

std::string OrchestrionModule::moduleName() const { return "Orchestrion"; }

void OrchestrionModule::resolveImports()
{
  const auto ar = ioc()->resolve<muse::ui::IUiActionsRegister>(moduleName());
  if (ar)
    ar->reg(m_uiActions);
}

void OrchestrionModule::registerExports()
{
  ioc()->registerExport<IOrchestrion>(moduleName(), m_orchestrion);
  ioc()->registerExport<IComputerKeyboardMidiController>(
      moduleName(), new ComputerKeyboardMidiController());
  ioc()->registerExport<IComputerKeyboard>(moduleName(), m_keyboard);
  ioc()->registerExport<IOrchestrionSequencerUiActions>(moduleName(),
                                                        m_uiActions);
}

void OrchestrionModule::onPreInit(const muse::IApplication::RunMode &)
{
  m_keyboard->preInit();
}

void OrchestrionModule::onInit(const muse::IApplication::RunMode &)
{
  m_orchestrion->init();
  m_actionController->init();
  m_keyboardController->init();
}
} // namespace dgk