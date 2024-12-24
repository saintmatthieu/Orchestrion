#pragma once

#include "DeviceMenuManager.h"
#include "IMidiControllerManager.h"
#include "IMidiSynthesizerManager.h"
#include <audioplugins/iknownaudiopluginsregister.h>
#include <midi/imidioutport.h>

#include <optional>

namespace dgk::orchestrion
{
class MidiSynthesizerMenuManager : public IMidiSynthesizerManager,
                                   public DeviceMenuManager
{
public:
  MidiSynthesizerMenuManager();

  void onInit();
  void onAllInited();

private:
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort = {this};
  muse::Inject<muse::audioplugins::IKnownAudioPluginsRegister> knownPlugins = {
      this};

  // IMidiSynthesizerManager
private:
  void trySelectDefaultDevice() override;
  SynthType synthType() const override;
  muse::async::Notification synthTypeChanged() const override;

  // DeviceMenuManager
private:
  std::string getMenuId(int deviceIndex) const override;
  std::string selectedDevice() const override;
  muse::async::Notification availableDevicesChanged() const override;
  std::vector<DeviceDesc> availableDevices() const override;
  bool selectDevice(const std::string &deviceId) override;

  std::vector<muse::audioplugins::AudioPluginInfo> availableSynths() const;

  muse::async::Notification m_availableDevicesChanged;
  muse::async::Notification m_selectedSynthChanged;
  std::vector<muse::audioplugins::AudioPluginInfo> m_availableSynths;
  std::optional<std::string> m_selectedSynth;
};
} // namespace dgk::orchestrion