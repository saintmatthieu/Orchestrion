#pragma once

#include "ISynthesizerManager.h"
#include "OrchestrionSynthesis/ISynthesizerConnector.h"
#include <async/asyncable.h>
#include <audio/iplayback.h>
#include <audioplugins/iknownaudiopluginsregister.h>
#include <midi/imidioutport.h>
#include <modularity/ioc.h>

namespace dgk::orchestrion
{
class SynthesizerManager : public ISynthesizerManager,
                           public muse::Injectable,
                           public muse::async::Asyncable
{
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort = {this};
  muse::Inject<muse::audioplugins::IKnownAudioPluginsRegister> knownPlugins = {
      this};
  muse::Inject<muse::audio::IPlayback> playback = {this};
  muse::Inject<ISynthesizerConnector> synthesizerConnector = {this};

public:
  SynthesizerManager() = default;
  void init();
  void onAllInited();

  // ISynthesizerManager
private:
  std::vector<DeviceDesc> availableSynths() const override;
  muse::async::Notification availableSynthsChanged() const override;
  bool selectSynth(const std::string &synthId) override;
  std::string selectedSynth() const override;

  std::vector<muse::audioplugins::AudioPluginInfo> availableInstruments() const;

  muse::async::Notification m_availableSynthsChanged;
  std::vector<muse::audioplugins::AudioPluginInfo> m_availableSynths;
  std::optional<std::string> m_selectedSynth;
};
} // namespace dgk::orchestrion