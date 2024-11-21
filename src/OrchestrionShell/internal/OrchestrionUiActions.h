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

#include "IOrchestrionUiActions.h"
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "async/asyncable.h"
#include "audio/iaudiodriver.h"
#include "modularity/ioc.h"
#include "ui/iuiactionsmodule.h"

namespace dgk::orchestrion
{
class OrchestrionUiActions : public IOrchestrionUiActions,
                             public muse::ui::IUiActionsModule,
                             public muse::async::Asyncable,
                             public muse::Injectable,
                             public muse::actions::Actionable
{
  muse::Inject<muse::audio::IAudioDriver> audioDriver = {this};
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher = {this};

public:
  OrchestrionUiActions();

  void init();

  // IOrchestrionUiActions
private:
  muse::async::Notification settablePlaybackDevicesChanged() const override;
  std::vector<DeviceAction> settablePlaybackDevices() const override;

  // muse::ui::IUiActionsModule
private:
  const muse::ui::UiActionList &actionsList() const override;
  bool actionEnabled(const muse::ui::UiAction &act) const override;
  muse::async::Channel<muse::actions::ActionCodeList>
  actionEnabledChanged() const override;

  bool actionChecked(const muse::ui::UiAction &act) const override;
  muse::async::Channel<muse::actions::ActionCodeList>
  actionCheckedChanged() const override;

  void fillDeviceCache();

private:
  struct DeviceItem
  {
    std::string id;
    std::string name;
    bool available = true;
  };

  const muse::ui::UiActionList m_actions;
  muse::async::Channel<muse::actions::ActionCodeList> m_actionEnabledChanged;
  muse::async::Channel<muse::actions::ActionCodeList> m_actionCheckedChanged;
  muse::async::Notification m_settablePlaybackDevicesChanged;
  std::vector<DeviceItem> m_deviceCache;
};
} // namespace dgk::orchestrion
