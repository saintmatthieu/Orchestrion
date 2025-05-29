#pragma once

#include "DeviceMenuManager.h"
#include "OrchestrionSynthesis/ISynthesizerManager.h"

namespace dgk
{
class SynthesizerMenuManager : public DeviceMenuManager
{
public:
  SynthesizerMenuManager();

  void onAllInited();

private:
  muse::Inject<ISynthesizerManager> synthManager = {this};

  // DeviceMenuManager
private:
  std::string getMenuId(int deviceIndex) const override;
  std::string selectedDevice() const override;
  muse::async::Notification availableDevicesChanged() const override;
  std::vector<DeviceDesc> availableDevices() const override;
  bool selectDevice(const std::string &deviceId) override;
};
} // namespace dgk