#include "SynthesizerMenuManager.h"

namespace dgk
{
SynthesizerMenuManager::SynthesizerMenuManager()
    : DeviceMenuManager{DeviceType::MidiSynthesizer}
{
}

std::string SynthesizerMenuManager::getMenuId(int deviceIndex) const
{
  return "chooseMidiSynth_" + std::to_string(deviceIndex);
}

std::string SynthesizerMenuManager::selectedDevice() const
{
  return synthManager()->selectedSynth();
}

muse::async::Notification
SynthesizerMenuManager::availableDevicesChanged() const
{
  return synthManager()->availableSynthsChanged();
}

std::vector<DeviceDesc> SynthesizerMenuManager::availableDevices() const
{
  return synthManager()->availableSynths();
}

bool SynthesizerMenuManager::selectDevice(const std::string &deviceId)
{
  return synthManager()->selectSynth(deviceId);
}

} // namespace dgk