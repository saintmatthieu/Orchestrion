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
#include "IOutputLatency.h"
#include "OrchestrionCommon/LatencyTrace.h"
#include "OrchestrionCommon/OfflineRendering.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "internal/BuiltInEffects.h"
#include "internal/DeviceFollowingAudioDriver.h"
#include "internal/EffectChain.h"
#include "internal/OfflineAudioDriver.h"
#include "internal/OrchestrionFxResolver.h"
#include "internal/OrchestrionSynthesisConfiguration.h"
#include "internal/OutputLatency.h"
#include "internal/SynthesizerConnector.h"
#include "internal/SynthesizerManager.h"
#include "internal/TracingAudioDriver.h"
#include "internal/TrackChannelMapper.h"
#include "view/EffectHeaderModel.h"
#include "view/EffectMeterModel.h"
#include "view/GlitchHintModel.h"
#include "view/OutputLatencyModel.h"
#include <audio/common/workmode.h>
#include <global/settings.h>
#include <interactive/iinteractiveuriregister.h>
#include <muse_framework_config.h>

#include <QQmlEngine>

namespace dgk
{
OrchestrionSynthesisModule::OrchestrionSynthesisModule()
    : m_synthesizerConnector{std::make_shared<SynthesizerConnector>()},
      m_synthesizerManager{std::make_shared<SynthesizerManager>()},
      m_configuration{std::make_shared<OrchestrionSynthesisConfiguration>()},
      m_builtInEffects{std::make_shared<BuiltInEffects>()},
      m_fxResolver{
          std::make_shared<OrchestrionFxResolver>(m_builtInEffects->runtime())},
      m_effectChain{std::make_shared<EffectChain>(m_fxResolver)},
      m_outputLatency{std::make_shared<OutputLatency>()}
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
  globalIoc()->registerExport<IEffectChain>(moduleName(), m_effectChain);
  globalIoc()->registerExport<IBuiltInEffects>(moduleName(), m_builtInEffects);
  globalIoc()->registerExport<IOutputLatency>(moduleName(), m_outputLatency);

  // Rendering a video offline, the sound goes to a file
  if (!offlineRendering.audioPath.empty())
  {
    globalIoc()->unregister<muse::audio::IAudioDriverFactory>(moduleName());
    globalIoc()->registerExport<muse::audio::IAudioDriverFactory>(
        moduleName(), std::make_shared<OfflineAudioDriverFactory>());
  }
  else if (std::shared_ptr<muse::audio::IAudioDriverFactory> factory =
               globalIoc()->resolve<muse::audio::IAudioDriverFactory>(
                   moduleName()))
  {
    // The platform's drivers, watched for underruns and given their margin
    factory =
        std::make_shared<DriverCapturingFactory>(factory, m_outputLatency);
    // following the output device
    factory = std::make_shared<DeviceFollowingAudioDriverFactory>(factory);
    // Tracing latencies, the callbacks are timed
    if (LatencyTrace::instance())
      factory = std::make_shared<TracingAudioDriverFactory>(factory);
    globalIoc()->unregister<muse::audio::IAudioDriverFactory>(moduleName());
    globalIoc()->registerExport<muse::audio::IAudioDriverFactory>(moduleName(),
                                                                  factory);
  }
}

void OrchestrionSynthesisModule::onPreInit(const muse::IApplication::RunMode &)
{
  // Rendering a video offline, all the engine's work is done in the driver's
  // callback (its "driver mode"), which OfflineAudioDriver calls when the
  // clock says: none of it on a timer of its own.
  if (!offlineRendering.audioPath.empty())
    muse::audio::workmode::setSessionMode(muse::audio::workmode::DriverMode);

#ifdef Q_OS_MACOS
  // CoreAudio's direct driver, an IOProc on the device, rather than the
  // AudioQueue, which keeps a second buffer queued: a whole block more of
  // latency. The audio module sets its own default and opens the driver in
  // the same onInit, so this comes first, now that the settings are loaded:
  // a key without a value yet takes its first default as its value, and the
  // audio module's ("CoreAudio") stays the default, the driver it falls back
  // on if this one fails to open. A stored value still wins.
  muse::settings()->setDefaultValue(
      muse::Settings::Key("audio", "io/audioDriver"),
      muse::Val("CoreAudioDirect"));
  // The engine's block, the least of the latency's levels: the device's
  // buffer is the block or a multiple of it (see IOutputLatency), so each
  // callback renders what it asks for, as on Linux.
  muse::settings()->setDefaultValue(
      muse::Settings::Key("audio", "io/bufferSize"), muse::Val(256));
#elif defined(MUSE_MODULE_AUDIO_PIPEWIRE)
  // PipeWire's own client, which asks for a latency (node.latency): the graph's
  // quantum comes down to the lowest asked. Through ALSA, a PipeWire system
  // plays at the graph's quantum. Without a PipeWire server, or with
  // MUSESCORE_FORCE_ALSA set, MuseScore's factory makes an ALSA driver anyway;
  // ALSA stays the default, as above.
  muse::settings()->setDefaultValue(
      muse::Settings::Key("audio", "io/audioDriver"), muse::Val("PipeWire"));
  // The engine's block, the least of the latency's levels: the quantum asked
  // for is the block or a multiple of it (see IOutputLatency), so each
  // callback renders what it asks for, never through the alignment buffer.
  muse::settings()->setDefaultValue(
      muse::Settings::Key("audio", "io/bufferSize"), muse::Val(256));
#elif defined(Q_OS_WIN)
  // MuseScore's 1024 frames are more than WASAPI's shared-mode period (10 ms,
  // 480 frames at 48 kHz): the engine rendered on every second or third
  // callback, through the alignment buffer, a note waiting up to 30 ms for
  // its block, and WASAPI's buffer was as long. Below the period, the engine
  // renders what each callback asks for, and the driver asks for a shorter
  // period, the shortest holding the block (low-latency shared mode), where
  // the device allows. 256 is the smallest MuseScore offers on Windows. Set
  // here, before the audio module's onInit, as on macOS above.
  muse::settings()->setDefaultValue(
      muse::Settings::Key("audio", "io/bufferSize"), muse::Val(256));
#endif
}

void OrchestrionSynthesisModule::registerUiTypes()
{
  qmlRegisterType<EffectMeterModel>("Orchestrion.OrchestrionSynthesis", 1, 0,
                                    "EffectMeterModel");
  qmlRegisterType<EffectHeaderModel>("Orchestrion.OrchestrionSynthesis", 1, 0,
                                     "EffectHeaderModel");
  qmlRegisterType<OutputLatencyModel>("Orchestrion.OrchestrionSynthesis", 1, 0,
                                      "OutputLatencyModel");
  qmlRegisterType<GlitchHintModel>("Orchestrion.OrchestrionSynthesis", 1, 0,
                                   "GlitchHintModel");
}

void OrchestrionSynthesisModule::resolveImports()
{
  // The built-in effects' window, from the Effects menu.
  auto ir = globalIoc()->resolve<muse::interactive::IInteractiveUriRegister>(
      moduleName());
  if (ir)
  {
    ir->registerQmlUri(muse::Uri("orchestrion://effects/builtin"),
                       "Orchestrion", "BuiltInEffectDialog");
    // Audio/MIDI > Latency…
    ir->registerQmlUri(muse::Uri(latencyDialogUri), "Orchestrion",
                       "LatencyDialog");
  }
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
  m_outputLatency->start();

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
