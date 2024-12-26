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
#include "OrchestrionSequencerUiActions.h"
#include <context/shortcutcontext.h>
#include <context/uicontext.h>
#include <global/log.h>

namespace dgk
{
namespace
{
muse::ui::UiActionList getActionList(
    const std::unordered_map<IComputerKeyboard::Layout, std::string> &map)
{
  {
    muse::ui::UiActionList list{
        muse::ui::UiAction("left-hand", mu::context::UiCtxNotationOpened,
                           mu::context::CTX_NOTATION_OPENED),
        muse::ui::UiAction("right-hand", mu::context::UiCtxNotationOpened,
                           mu::context::CTX_NOTATION_OPENED)};

    for (const auto &[_, id] : map)
      list.emplace_back(id, mu::context::UiCtxAny, mu::context::CTX_ANY);

    return list;
  }
}
} // namespace

OrchestrionSequencerUiActions::OrchestrionSequencerUiActions()
    : m_actions{getActionList(computerKeyboardSetterActionIds())}
{
}

std::unordered_map<IComputerKeyboard::Layout, std::string>
OrchestrionSequencerUiActions::computerKeyboardSetterActionIds() const
{
  std::unordered_map<IComputerKeyboard::Layout, std::string> ids;
  const std::vector<std::string> layoutNames =
      IComputerKeyboard::availableLayouts();
  ids.reserve(layoutNames.size());
  for (const auto &layoutName : layoutNames)
  {
    const auto layout = IComputerKeyboard::layoutFromString(layoutName);
    IF_ASSERT_FAILED(layout) { continue; }
    const auto id = "keyboard-language-set-" + layoutName;
    ids.emplace(*layout, id);
  }
  return ids;
}

const muse::ui::UiActionList &OrchestrionSequencerUiActions::actionsList() const
{
  return m_actions;
}

bool OrchestrionSequencerUiActions::actionEnabled(
    const muse::ui::UiAction &) const
{
  return true;
}

muse::async::Channel<muse::actions::ActionCodeList>
OrchestrionSequencerUiActions::actionEnabledChanged() const
{
  static muse::async::Channel<muse::actions::ActionCodeList> channel;
  return channel;
}

bool OrchestrionSequencerUiActions::actionChecked(
    const muse::ui::UiAction &) const
{
  return false;
}

muse::async::Channel<muse::actions::ActionCodeList>
OrchestrionSequencerUiActions::actionCheckedChanged() const
{
  static muse::async::Channel<muse::actions::ActionCodeList> channel;
  return channel;
}
} // namespace dgk
