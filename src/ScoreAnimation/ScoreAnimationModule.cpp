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
#include "ScoreAnimationModule.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "internal/ScoreAnimator.h"
#include "internal/SegmentRegistry.h"

namespace dgk
{
ScoreAnimationModule::ScoreAnimationModule()
    : m_scoreAnimator(std::make_shared<ScoreAnimator>())
{
}

std::string ScoreAnimationModule::moduleName() const
{
  return "ScoreAnimation";
}

void ScoreAnimationModule::registerExports()
{
  globalIoc()->registerExport<IScoreAnimator>(moduleName(), m_scoreAnimator);
  globalIoc()->registerExport<ISegmentRegistry>(moduleName(),
                                                new SegmentRegistry());
}

muse::modularity::IContextSetup *ScoreAnimationModule::newContext(
    const muse::modularity::ContextPtr &ctx) const
{
  ModuleContextSetup::Hooks hooks;
  hooks.onInit = [this](const muse::IApplication::RunMode &mode)
  { onContextInit(mode); };
  return new ModuleContextSetup(ctx, std::move(hooks));
}

void ScoreAnimationModule::onContextInit(
    const muse::IApplication::RunMode &) const
{
  m_scoreAnimator->init();
}
} // namespace dgk
