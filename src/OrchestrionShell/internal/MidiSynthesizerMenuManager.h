#pragma once

#include "DeviceMenuManager.h"
#include "IMidiControllerManager.h"
#include "IMidiSynthesizerManager.h"
#include <midi/IMidiOutPort.h>

namespace dgk::orchestrion
{
class MidiSynthesizerMenuManager : public IMidiSynthesizerManager,
                                   public DeviceMenuManager
{
public:
  MidiSynthesizerMenuManager();

private:
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort = {this};

  // IMidiSynthesizerManager
private:
  void trySelectDefaultDevice() override;

  // DeviceMenuManager
private:
  std::string getMenuId(int deviceIndex) const override;
  std::string selectedDevice() const override;
  muse::async::Notification availableDevicesChanged() const override;
  std::vector<DeviceDesc> availableDevices() const override;
  bool selectDevice(const std::string &deviceId) override;
};
} // namespace dgk::orchestrion