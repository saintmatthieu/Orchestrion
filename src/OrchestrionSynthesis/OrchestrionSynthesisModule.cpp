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
#include "OrchestrionSynthesisModule.h"
#include "IBuiltInEffects.h"
#include "IEffectChain.h"
#include "IOrchestrionSynthesisConfiguration.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "internal/BuiltInEffects.h"
#include "internal/EffectChain.h"
#include "internal/OrchestrionFxResolver.h"
#include "internal/OrchestrionSynthesisConfiguration.h"
#include "internal/SynthesizerConnector.h"
#include "internal/SynthesizerManager.h"
#include "internal/TrackChannelMapper.h"
#include "view/EffectMeterModel.h"
#include "view/EffectHeaderModel.h"
#include <interactive/iinteractiveuriregister.h>

#include <QQmlEngine>

namespace dgk
{
OrchestrionSynthesisModule::OrchestrionSynthesisModule()
    : m_synthesizerConnector{std::make_shared<SynthesizerConnector>()},
      m_synthesizerManager{std::make_shared<SynthesizerManager>()},
      m_configuration{std::make_shared<OrchestrionSynthesisConfiguration>()},
      m_builtInEffects{std::make_shared<BuiltInEffects>()},
      m_fxResolver{std::make_shared<OrchestrionFxResolver>(
          m_builtInEffects->runtime())},
      m_effectChain{std::make_shared<EffectChain>(m_fxResolver)}
{
}

std::string OrchestrionSynthesisModule::moduleName() const
{
  return "OrchestrionSynthesis";
}

void OrchestrionSynthesisModule::registerExports()
{
  globalIoc()->registerExport<ISynthesizerConnector>(moduleName(),
                                                     m_synthesizerConnector);
  globalIoc()->registerExport<ITrackChannelMapper>(moduleName(),
                                                   new TrackChannelMapper);
  globalIoc()->registerExport<ISynthesizerManager>(moduleName(),
                                                   m_synthesizerManager);
  globalIoc()->registerExport<IOrchestrionSynthesisConfiguration>(
      moduleName(), m_configuration);
  globalIoc()->registerExport<IEffectChain>(moduleName(), m_effectChain);
  globalIoc()->registerExport<IBuiltInEffects>(moduleName(),
                                               m_builtInEffects);
}

void OrchestrionSynthesisModule::registerUiTypes()
{
  qmlRegisterType<EffectMeterModel>("Orchestrion.OrchestrionSynthesis", 1, 0,
                                    "EffectMeterModel");
  qmlRegisterType<EffectHeaderModel>("Orchestrion.OrchestrionSynthesis", 1, 0,
                                     "EffectHeaderModel");
}

void OrchestrionSynthesisModule::resolveImports()
{
  // The built-in effects' window, from the Effects menu.
  auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(
      moduleName());
  if (ir)
    ir->registerQmlUri(muse::Uri("orchestrion://effects/builtin"),
                       "Orchestrion", "BuiltInEffectDialog");
}

void OrchestrionSynthesisModule::onDelayedInit()
{
  m_configuration->postInit();
}

muse::modularity::IContextSetup *OrchestrionSynthesisModule::newContext(
    const muse::modularity::ContextPtr &ctx) const
{
  ModuleContextSetup::Hooks hooks;
  hooks.onInit = [this](const muse::IApplication::RunMode &mode)
  { onContextInit(mode); };
  hooks.onAllInited = [this](const muse::IApplication::RunMode &mode)
  { onContextAllInited(mode); };
  return new ModuleContextSetup(ctx, std::move(hooks));
}

void OrchestrionSynthesisModule::onContextInit(
    const muse::IApplication::RunMode &) const
{
  m_configuration->init();
  m_synthesizerManager->init();
  m_builtInEffects->init();
}

void OrchestrionSynthesisModule::onContextAllInited(
    const muse::IApplication::RunMode &) const
{
  m_synthesizerManager->onAllInited();
  m_synthesizerConnector->onAllInited();
  m_effectChain->onAllInited();

  // The VST editor window gets Orchestrion's header (bypass) around the
  // framework's editor item. Replaced here, after the vst module's context
  // init has registered its own dialog for the URI.
  auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(
      moduleName());
  if (ir)
  {
    const muse::Uri uri("muse://vst/editor");
    ir->unregisterUri(uri);
    ir->registerQmlUri(uri, "Orchestrion", "OrchestrionVstEditorDialog");
  }
}
} // namespace dgk
