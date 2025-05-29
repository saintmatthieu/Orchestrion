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

#include "GestureControllers/IComputerKeyboard.h"
#include "IOrchestrionSequencerUiActions.h"
#include <ui/iuiactionsmodule.h>
#include <ui/uiaction.h>

namespace dgk
{
class OrchestrionSequencerUiActions : public muse::ui::IUiActionsModule,
                                      public IOrchestrionSequencerUiActions
{
public:
  OrchestrionSequencerUiActions();

  // IOrchestrionSequencerUiActions
private:
  std::unordered_map<IComputerKeyboard::Layout, std::string>
  computerKeyboardSetterActionIds() const override;

  // muse::ui::IUiActionsModule
private:
  const muse::ui::UiActionList &actionsList() const override;
  bool actionEnabled(const muse::ui::UiAction &act) const override;
  muse::async::Channel<muse::actions::ActionCodeList>
  actionEnabledChanged() const override;

  bool actionChecked(const muse::ui::UiAction &act) const override;
  muse::async::Channel<muse::actions::ActionCodeList>
  actionCheckedChanged() const override;

private:
  const muse::ui::UiActionList m_actions;
};
} // namespace dgk
