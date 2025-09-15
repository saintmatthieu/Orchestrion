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

#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <context/iglobalcontext.h>
#include <global/iglobalconfiguration.h>
#include <global/iinteractive.h>
#include <global/modularity/ioc.h>
#include <project/iprojectconfiguration.h>

namespace dgk
{
class OrchestrionActionController : public muse::actions::Actionable,
                                    public muse::Injectable
{
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<muse::IGlobalConfiguration> globalConfiguration;
  muse::Inject<mu::project::IProjectConfiguration> configuration;
  muse::Inject<muse::IInteractive> interactive;

public:
  void init();
  void onFileOpen() const;
  void openFromDir(const muse::io::path_t &dir) const;
  void closeCurrent() const;
};
} // namespace dgk