#include "MidiSynthesizerMenuManager.h"

namespace dgk::orchestrion
{
MidiSynthesizerMenuManager::MidiSynthesizerMenuManager()
    : DeviceMenuManager{DeviceType::MidiSynthesizer}
{
}

void MidiSynthesizerMenuManager::trySelectDefaultDevice()
{
  DeviceMenuManager::doTrySelectDefaultDevice();
}

std::string MidiSynthesizerMenuManager::getMenuId(int deviceIndex) const
{
  return "chooseMidiSynth_" + std::to_string(deviceIndex);
}

std::string MidiSynthesizerMenuManager::selectedDevice() const
{
  return midiOutPort()->deviceID();
}

muse::async::Notification
MidiSynthesizerMenuManager::availableDevicesChanged() const
{
  return midiOutPort()->availableDevicesChanged();
}

std::vector<DeviceMenuManager::DeviceDesc>
MidiSynthesizerMenuManager::availableDevices() const
{
  const auto midiDevices = midiOutPort()->availableDevices();
  std::vector<DeviceDesc> descriptions(midiDevices.size());
  std::transform(midiDevices.begin(), midiDevices.end(), descriptions.begin(),
                 [](const auto &device)
                 { return DeviceDesc{device.id, device.name}; });
  // TODO get all other available synths
  return descriptions;
}

bool MidiSynthesizerMenuManager::selectDevice(const std::string &deviceId)
{
  if (midiOutPort()->connect(deviceId))
  {
    onDeviceSuccessfullySet(deviceId);
    return true;
  }
  return false;
}

} // namespace dgk::orchestrion