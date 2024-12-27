#include "SynthesizerManager.h"
#include <audio/itracks.h>
#include <log.h>

namespace dgk::orchestrion
{
namespace
{
constexpr auto builtinId = "builtin";
}

void SynthesizerManager::init()
{
  midiOutPort()->availableDevicesChanged().onNotify(
      this, [this] { m_availableSynthsChanged.notify(); });
}

void SynthesizerManager::onAllInited() { m_availableSynthsChanged.notify(); }

std::vector<DeviceDesc> SynthesizerManager::availableSynths() const
{
  using namespace muse;
  const std::vector<midi::MidiDevice> midiDevices =
      midiOutPort()->availableDevices();
  const std::vector<audioplugins::AudioPluginInfo> instruments =
      availableInstruments();
  std::vector<DeviceDesc> descriptions;
  descriptions.reserve(1u + midiDevices.size() + instruments.size());

  // built-in
  descriptions.emplace_back(DeviceDesc{builtinId, "Built-in"});

  // plugins
  std::transform(instruments.begin(), instruments.end(),
                 std::back_inserter(descriptions),
                 [](const audioplugins::AudioPluginInfo &synth)
                 { return DeviceDesc{synth.meta.id, synth.meta.id}; });

  // MIDI devices
  std::transform(midiDevices.begin(), midiDevices.end(),
                 std::back_inserter(descriptions), [](const auto &device)
                 { return DeviceDesc{device.id, device.name}; });

  return descriptions;
}

muse::async::Notification SynthesizerManager::availableSynthsChanged() const
{
  return m_availableSynthsChanged;
}

bool SynthesizerManager::selectSynth(const std::string &synthId)
{
  const auto instruments = availableInstruments();
  if (const auto it = std::find_if(
          instruments.begin(), instruments.end(),
          [&synthId](const muse::audioplugins::AudioPluginInfo &instrument)
          { return instrument.meta.id == synthId; });
      it != instruments.end())
  {
    synthesizerConnector()->connectSynthesizer(it->meta);
    midiOutPort()->disconnect();
    m_selectedSynth = synthId;

    return true;
  }

  const auto midiDevices = midiOutPort()->availableDevices();
  if (std::any_of(midiDevices.begin(), midiDevices.end(),
                  [&synthId](const auto &device)
                  { return device.id == synthId; }))
  {
    midiOutPort()->connect(synthId);
    m_selectedSynth = synthId;
    return true;
  }

  m_selectedSynth.reset();
  // Also return true - we'll be using the built-in synth
  return true;
}

std::string SynthesizerManager::selectedSynth() const
{
  return m_selectedSynth.value_or(builtinId);
}

std::vector<muse::audioplugins::AudioPluginInfo>
SynthesizerManager::availableInstruments() const
{
  return knownPlugins()->pluginInfoList(
      [](const muse::audioplugins::AudioPluginInfo &info)
      { return info.type == muse::audioplugins::AudioPluginType::Instrument; });
}

} // namespace dgk::orchestrion