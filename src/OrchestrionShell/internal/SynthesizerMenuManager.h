#pragma once

#include "DeviceMenuManager.h"
#include "ISynthesizerManager.h"
#include "ISynthesizerMenuManager.h"

namespace dgk
{
class SynthesizerMenuManager : public ISynthesizerMenuManager,
                               public DeviceMenuManager
{
public:
  SynthesizerMenuManager();

  void onAllInited();

private:
  muse::Inject<ISynthesizerManager> synthManager = {this};

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
};
} // namespace dgk