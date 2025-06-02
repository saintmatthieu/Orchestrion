#include "AudioDeviceService.h"

#include <future>
#include <thread>

namespace dgk
{
void AudioDeviceService::init()
{
  audioDriver()->outputDeviceChanged().onNotify(
      this,
      [this]
      {
        if (!m_postInitCalled)
          return;

        if (m_deviceChangeExpected)
        {
          m_deviceChangeExpected = false;
          configuration()->writeSelectedAudioDevice(selectedDevice());
          m_selectedDeviceChanged.notify();
        }
        else
          selectDevice(configuration()->readSelectedAudioDevice());
      });
  audioDriver()->availableOutputDevicesChanged().onNotify(this, [this] {

  });
}

void AudioDeviceService::postInit()
{
  m_postInitCalled = true;
  const std::optional<ExternalDeviceId> configDevice =
      configuration()->readSelectedAudioDevice();
  selectDevice(configDevice.value_or(ExternalDeviceId{"default"}));
}

void AudioDeviceService::selectDefaultDevice()
{
  selectDevice(ExternalDeviceId{"default"});
}

void AudioDeviceService::selectDevice(const std::optional<ExternalDeviceId> &id)
{
  m_deviceChangeExpected = true;
  std::promise<void> p;
  auto f = p.get_future();
  std::thread t{[&]
                {
                  audioDriver()->selectOutputDevice(
                      id.value_or(ExternalDeviceId{""}).value);
                  p.set_value();
                }};
  f.get();
  t.join();
}

std::vector<ExternalDeviceId> AudioDeviceService::availableDevices() const
{
  const auto devices = museAvailableDevices();
  std::vector<ExternalDeviceId> ids;
  ids.reserve(devices.size());
  for (const auto &device : devices)
    ids.emplace_back(device.id);
  return ids;
}

std::vector<muse::audio::AudioDevice>
AudioDeviceService::museAvailableDevices() const
{
  std::promise<std::vector<muse::audio::AudioDevice>> p;
  auto f = p.get_future();
  std::thread t{[this, &p]
                { p.set_value(audioDriver()->availableOutputDevices()); }};
  auto devices = f.get();
  t.join();
  return devices;
}

bool AudioDeviceService::isAvailable(const ExternalDeviceId &query) const
{
  const auto devices = availableDevices();
  return std::any_of(devices.begin(), devices.end(),
                     [&query](const ExternalDeviceId &device)
                     { return device == query; });
}

muse::async::Notification AudioDeviceService::availableDevicesChanged() const
{
  return audioDriver()->availableOutputDevicesChanged();
}

std::optional<ExternalDeviceId> AudioDeviceService::selectedDevice() const
{
  const auto id = audioDriver()->outputDevice();
  if (id.empty())
    return {};

  return ExternalDeviceId{id};
}

muse::async::Notification AudioDeviceService::selectedDeviceChanged() const
{
  return m_selectedDeviceChanged;
}

std::string AudioDeviceService::deviceName(const ExternalDeviceId &id) const
{
  const auto devices = museAvailableDevices();
  const auto it =
      std::find_if(devices.begin(), devices.end(),
                   [&id](const auto &device) { return device.id == id.value; });
  if (it != devices.end())
    return it->name;
  return {};
}
} // namespace dgk