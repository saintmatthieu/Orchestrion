#include "AudioDeviceService.h"

#include <future>
#include <thread>

namespace dgk
{
namespace
{
const ExternalDeviceId defaultDevice{"default"};
}

void AudioDeviceService::init()
{
  audioDriver()->availableOutputDevicesChanged().onNotify(
      this,
      [this]
      {
        const auto configDevice = configuration()->readSelectedAudioDevice();
        if (!configDevice)
        {
          if (!selectedDevice())
          {
            m_deviceChangeExpected = true;
            doSelectDevice(defaultDevice);
          }
          return;
        }

        const auto available = isAvailable(*configDevice);
        if (available && selectedDevice() == configDevice)
          return;

        m_deviceChangeExpected = true;
        doSelectDevice(available ? *configDevice : defaultDevice);
      });

  audioDriver()->outputDeviceChanged().onNotify(
      this,
      [this]
      {
        if (!m_postInitCalled)
        {
          // We haven't done the post-init automatic selection yet, still, for
          // reliability, we should notify the change.
          m_selectedDeviceChanged.notify();
          return;
        }

        if (m_deviceChangeExpected)
        {
          m_deviceChangeExpected = false;
          m_selectedDeviceChanged.notify();
        }
        else
          // Undo what something unkown did.
          if (const auto configDevice =
                  configuration()->readSelectedAudioDevice())
          {
            m_deviceChangeExpected = true;
            doSelectDevice(*configDevice);
          }
          else
            // Oh well ... we still have to notify.
            m_selectedDeviceChanged.notify();
      });
}

void AudioDeviceService::postInit()
{
  m_postInitCalled = true;
  const std::optional<ExternalDeviceId> configDevice =
      configuration()->readSelectedAudioDevice();
  m_deviceChangeExpected = true;
  if (configDevice && isAvailable(*configDevice))
    doSelectDevice(*configDevice);
  else
    doSelectDevice(defaultDevice);
}

void AudioDeviceService::selectDefaultDevice() { selectDevice(defaultDevice); }

void AudioDeviceService::selectDevice(const std::optional<ExternalDeviceId> &id)
{
  m_deviceChangeExpected = true;
  configuration()->writeSelectedAudioDevice(id);
  doSelectDevice(id.value_or(ExternalDeviceId{""}));
}

void AudioDeviceService::doSelectDevice(const ExternalDeviceId &id)
{
  std::promise<void> p;
  auto f = p.get_future();
  std::thread t{[&]
                {
                  audioDriver()->selectOutputDevice(id.value);
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

bool AudioDeviceService::isNoDevice(const ExternalDeviceId &) const
{
  return false;
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