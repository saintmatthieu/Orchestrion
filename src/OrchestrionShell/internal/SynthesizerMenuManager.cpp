#include "SynthesizerMenuManager.h"

namespace dgk::orchestrion
{
namespace
{
constexpr auto builtinId = "builtin";
}
SynthesizerMenuManager::SynthesizerMenuManager()
    : DeviceMenuManager{DeviceType::MidiSynthesizer}
{
}

void SynthesizerMenuManager::onInit()
{
  midiOutPort()->availableDevicesChanged().onNotify(
      this, [this] { m_availableDevicesChanged.notify(); });
}

void SynthesizerMenuManager::onAllInited()
{
  m_availableDevicesChanged.notify();
}

void SynthesizerMenuManager::trySelectDefaultDevice()
{
  DeviceMenuManager::doTrySelectDefaultDevice();
}

std::string SynthesizerMenuManager::getMenuId(int deviceIndex) const
{
  return "chooseMidiSynth_" + std::to_string(deviceIndex);
}

std::string SynthesizerMenuManager::selectedDevice() const
{
  return m_selectedSynth.value_or(builtinId);
}

muse::async::Notification
SynthesizerMenuManager::availableDevicesChanged() const
{
  return m_availableDevicesChanged;
}

std::vector<DeviceDesc> SynthesizerMenuManager::availableDevices() const
{
  using namespace muse;
  const std::vector<midi::MidiDevice> midiDevices =
      midiOutPort()->availableDevices();
  const std::vector<audioplugins::AudioPluginInfo> synths = availableSynths();
  std::vector<DeviceDesc> descriptions;
  descriptions.reserve(1u + midiDevices.size() + synths.size());

  // built-in
  descriptions.emplace_back(DeviceDesc{builtinId, "Built-in"});

  // plugins
  std::transform(synths.begin(), synths.end(), std::back_inserter(descriptions),
                 [](const audioplugins::AudioPluginInfo &synth)
                 { return DeviceDesc{synth.meta.id, synth.meta.id}; });

  // MIDI devices
  std::transform(midiDevices.begin(), midiDevices.end(),
                 std::back_inserter(descriptions), [](const auto &device)
                 { return DeviceDesc{device.id, device.name}; });

  return descriptions;
}

std::vector<muse::audioplugins::AudioPluginInfo>
SynthesizerMenuManager::availableSynths() const
{
  return knownPlugins()->pluginInfoList(
      [](const muse::audioplugins::AudioPluginInfo &info)
      { return info.type == muse::audioplugins::AudioPluginType::Instrument; });
}

bool SynthesizerMenuManager::selectDevice(const std::string &deviceId)
{
  const auto synths = availableSynths();
  const auto synthIt =
      std::find_if(synths.begin(), synths.end(),
                   [&deviceId](const muse::audioplugins::AudioPluginInfo &synth)
                   { return synth.meta.id == deviceId; });
  if (synthIt != synths.end())
  {
    midiOutPort()->disconnect();
    m_selectedSynth = deviceId;
  }
  else if (midiOutPort()->connect(deviceId))
    m_selectedSynth = deviceId;
  else
    m_selectedSynth.reset();

  onDeviceSuccessfullySet(deviceId);
  return true;
}

} // namespace dgk::orchestrion