#include "MidiDeviceService.h"

namespace dgk
{
using namespace ExternalDevicesUtils;

void MidiDeviceService::init()
{
  midiInPort()->deviceChanged().onNotify(
      this,
      [this]
      {
        const auto configDevice = configuration()->readSelectedMidiDevice();
        const auto selectedDevice = this->selectedDevice();
        if (configDevice == selectedDevice)
          return;

        if (m_deviceChangeExpected)
        {
          configuration()->writeSelectedMidiDevice(selectedDevice);
          m_selectedDeviceChanged.notify();
        }
        else
        {
          ScopedTrue scope{m_deviceChangeExpected};
          // MuseScore's midimodule reads from its own configuration on startup.
          // Catch this call and reset the device to our configuration.
          selectDeviceWhileExpecting(configDevice);
        }
      });
}

void MidiDeviceService::postInit()
{
  selectDevice(configuration()->readSelectedMidiDevice());
}

std::vector<ExternalDeviceId> MidiDeviceService::availableDevices() const
{
  const muse::midi::MidiDeviceList midiDevices =
      midiInPort()->availableDevices();
  std::vector<ExternalDeviceId> ids;
  ids.reserve(midiDevices.size());
  for (const auto &device : midiDevices)
    ids.emplace_back(device.id);
  return ids;
}

bool MidiDeviceService::isAvailable(const ExternalDeviceId &id) const
{
  if (id == "-1")
    return false;
  const muse::midi::MidiDeviceList midiDevices =
      midiInPort()->availableDevices();
  return std::any_of(midiDevices.begin(), midiDevices.end(),
                     [&id](const auto &device)
                     { return device.id == id.value; });
}

muse::async::Notification MidiDeviceService::availableDevicesChanged() const
{
  return midiInPort()->availableDevicesChanged();
}

std::optional<ExternalDeviceId> MidiDeviceService::selectedDevice() const
{
  if (!midiInPort()->isConnected())
    return {};

  const auto id = midiInPort()->deviceID();
  if (id.empty())
    return {};

  return ExternalDeviceId{id};
}

void MidiDeviceService::selectDevice(const std::optional<ExternalDeviceId> &id)
{
  ScopedTrue scope{m_deviceChangeExpected};
  selectDeviceWhileExpecting(id);
}

void MidiDeviceService::selectDeviceWhileExpecting(
    const std::optional<ExternalDeviceId> &id)
{
  if (id == selectedDevice())
    return;
  if (!id)
    midiInPort()->disconnect();
  else
    midiInPort()->connect(id->value);
}

muse::async::Notification MidiDeviceService::selectedDeviceChanged() const
{
  return m_selectedDeviceChanged;
}

std::string MidiDeviceService::deviceName(const ExternalDeviceId &id) const
{
  const muse::midi::MidiDeviceList midiDevices =
      midiInPort()->availableDevices();
  const auto it =
      std::find_if(midiDevices.begin(), midiDevices.end(),
                   [&id](const auto &device) { return device.id == id.value; });
  if (it != midiDevices.end())
    return it->name;
  return {};
}
} // namespace dgk