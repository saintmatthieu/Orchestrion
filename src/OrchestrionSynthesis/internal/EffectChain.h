/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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

#include "IEffectChain.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <audio/common/audiotypes.h>
#include <audio/engine/ifxresolver.h>
#include <audio/main/iplayback.h>
#include <audio/main/istartaudiocontroller.h>
#include <audioplugins/iknownaudiopluginsregister.h>
#include <interactive/iinteractive.h>
#include <modularity/ioc.h>
#include <playback/iplaybackcontroller.h>
#include <vst/ivstinstancesregister.h>

namespace dgk
{
class OrchestrionFxResolver;

/**
 * Owns the master effect chain: keeps MuseScore's audio engine fed with it,
 * takes the plugin-state changes made in the effects' editors back from the
 * engine, and persists the whole thing in the application settings.
 */
class EffectChain : public IEffectChain,
                    public dgk::Injectable,
                    public muse::async::Asyncable
{
  dgk::Inject<muse::audio::IPlayback> playback{this};
  dgk::Inject<mu::playback::IPlaybackController> playbackController{this};
  dgk::Inject<muse::audioplugins::IKnownAudioPluginsRegister> knownPlugins{
      this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<muse::vst::IVstInstancesRegister> vstInstancesRegister{this};
  dgk::Inject<muse::IInteractive> interactive{this};
  dgk::Inject<muse::audio::fx::IFxResolver> fxResolver{this};
  dgk::Inject<muse::audio::IStartAudioController> startAudioController{this};

public:
  explicit EffectChain(std::shared_ptr<OrchestrionFxResolver> resolver);

  void onAllInited();

  // IEffectChain
private:
  std::vector<EffectDesc> availableEffects() const override;
  muse::async::Notification availableEffectsChanged() const override;
  std::vector<EffectDesc> chain() const override;
  muse::async::Notification chainChanged() const override;
  bool contains(const std::string &effectId) const override;
  void addEffect(const std::string &effectId) override;
  void removeEffect(const std::string &effectId) override;
  bool isActive(const std::string &effectId) const override;
  void setActive(const std::string &effectId, bool active) override;
  void openEditor(const std::string &effectId) override;

private:
  std::vector<muse::audioplugins::AudioPluginInfo> availablePlugins() const;
  void onEngineChainChanged(const muse::audio::AudioFxChain &chain);
  void onChainModified();
  /** Sends the chain to the audio engine. */
  void apply();
  void load();
  void save() const;
  /** Hands the built-in effects' resolver to the audio engine. */
  void registerResolver();
  /**
   * Opens the editor of a just-added effect: at once for a built-in one,
   * for a VST once the engine has created and loaded its instance.
   */
  void openEditorWhenReady(const std::string &effectId, int attempt = 0);

  const std::shared_ptr<OrchestrionFxResolver> m_resolver;
  muse::audio::AudioFxChain m_chain;
  muse::async::Notification m_availableEffectsChanged;
  muse::async::Notification m_chainChanged;
};
} // namespace dgk
