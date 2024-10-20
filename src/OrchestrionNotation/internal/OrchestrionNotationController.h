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

#include "actions/iactionsdispatcher.h"
#include "async/notification.h"
#include "context/iglobalcontext.h"
#include "engraving/rendering/iscorerenderer.h"
#include "modularity/ioc.h"
#include "ui/iuiconfiguration.h"

namespace dgk::orchestrion
{
class OrchestrionNotationController
{
public:
  INJECT(muse::actions::IActionsDispatcher, dispatcher);
  INJECT(mu::context::IGlobalContext, globalContext);
  INJECT(mu::engraving::rendering::IScoreRenderer, scoreRenderer);
  INJECT(muse::ui::IUiConfiguration, uiConfiguration);

  void configureNotation();
  muse::async::Notification pageSizeChanged() const;
  mu::Size pageSize() const;

private:
  muse::async::Notification m_pageSizeChanged;
};
} // namespace dgk::orchestrion
