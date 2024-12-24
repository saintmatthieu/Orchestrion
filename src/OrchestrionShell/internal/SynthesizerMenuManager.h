#pragma once

#include "DeviceMenuManager.h"
#include "IControllerMenuManager.h"
#include "ISynthesizerMenuManager.h"
#include <audioplugins/iknownaudiopluginsregister.h>
#include <midi/imidioutport.h>

#include <optional>

namespace dgk::orchestrion
{
class SynthesizerMenuManager : public ISynthesizerMenuManager,
                               public DeviceMenuManager
{
public:
  SynthesizerMenuManager();

  void onInit();
  void onAllInited();

private:
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort = {this};
  muse::Inject<muse::audioplugins::IKnownAudioPluginsRegister> knownPlugins = {
      this};

  // ISynthesizerMenuManager
private:
  void trySelectDefaultDevice() override;

  // DeviceMenuManager
private:
  std::string getMenuId(int deviceIndex) const override;
  std::string selectedDevice() const override;
  muse::async::Notification availableDevicesChanged() const override;
  std::vector<DeviceDesc> availableDevices() const override;
  bool selectDevice(const std::string &deviceId) override;

  std::vector<muse::audioplugins::AudioPluginInfo> availableSynths() const;

  muse::async::Notification m_availableDevicesChanged;
  std::vector<muse::audioplugins::AudioPluginInfo> m_availableSynths;
  std::optional<std::string> m_selectedSynth;
};
} // namespace dgk::orchestrion