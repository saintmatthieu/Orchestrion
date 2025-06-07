#pragma once

#include "../ExternalDevicesUtils.h"
#include "IExternalDevicesConfiguration.h"
#include "IMidiDeviceService.h"

#include "async/asyncable.h"
#include "async/notification.h"
#include "midi/imidiinport.h"
#include "modularity/ioc.h"

namespace dgk
{
class MidiDeviceService : public IMidiDeviceService,
                          public muse::Injectable,
                          public muse::async::Asyncable
{
public:
  void init();
  void postInit();

  std::vector<ExternalDeviceId> availableDevices() const override;
  muse::async::Notification availableDevicesChanged() const override;
  bool isAvailable(const ExternalDeviceId &) const override;
  bool isNoDevice(const ExternalDeviceId &id) const override;

  void selectDevice(const std::optional<ExternalDeviceId> &) override;
  muse::async::Notification selectedDeviceChanged() const override;
  std::optional<ExternalDeviceId> selectedDevice() const override;

  void selectDefaultDevice() override;

  std::string deviceName(const ExternalDeviceId &) const override;

private:
  void doSelectDevice(const ExternalDeviceId &);
  std::vector<ExternalDeviceId> availableDevicesWithoutNoDevice() const;
  std::optional<ExternalDeviceId> selectedDeviceWithoutNoDevice() const;

  muse::Inject<muse::midi::IMidiInPort> midiInPort;
  muse::Inject<IExternalDevicesConfiguration> configuration;
  muse::async::Notification m_selectedDeviceChanged;
  bool m_deviceChangeExpected = false;
  bool m_postInitCalled = false;
};
} // namespace dgk