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

#include "OrchestrionShell/IControllerMenuManager.h"
#include "OrchestrionShell/IOrchestrionStartupScenario.h"
#include <QQuickItem>
#include <actions/iactionsdispatcher.h>
#include <global/iglobalconfiguration.h>
#include <modularity/ioc.h>
#include <multiinstances/imultiinstancesprovider.h>
#include <optional>

namespace dgk::orchestrion
{
class OrchestrionOnboardingModel : public QQuickItem, public muse::Injectable
{
  Q_OBJECT

  muse::Inject<muse::actions::IActionsDispatcher> dispatcher = {this};
  muse::Inject<muse::IGlobalConfiguration> globalConfiguration = {this};
  muse::Inject<IOrchestrionStartupScenario> startupScenario = {this};
  muse::Inject<muse::mi::IMultiInstancesProvider> multiInstances = {this};
  muse::Inject<IControllerMenuManager> midiControllerManager = {this};

public:
  Q_INVOKABLE void startOnboarding();
  Q_INVOKABLE void onGainedFocus();

  explicit OrchestrionOnboardingModel(QQuickItem *parent = nullptr);

private:
  std::optional<muse::actions::ActionData>
  getFileOpenArgs(const StartupProjectFile &) const;
};
} // namespace dgk::orchestrion