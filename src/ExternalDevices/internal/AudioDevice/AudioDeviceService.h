#pragma once

#include "IAudioDeviceService.h"
#include "IExternalDevicesConfiguration.h"

#include "async/async.h"
#include "async/asyncable.h"
#include "async/notification.h"
#include "audio/iaudiodriver.h"
#include "modularity/ioc.h"

namespace dgk
{
class AudioDeviceService : public IAudioDeviceService,
                           public muse::Injectable,
                           public muse::async::Asyncable
{
public:
  void init();
  void postInit();

  std::vector<ExternalDeviceId> availableDevices() const override;
  muse::async::Notification availableDevicesChanged() const override;
  bool isAvailable(const ExternalDeviceId &) const override;

  void selectDevice(const std::optional<ExternalDeviceId> &) override;
  muse::async::Notification selectedDeviceChanged() const override;
  std::optional<ExternalDeviceId> selectedDevice() const override;

  std::string deviceName(const ExternalDeviceId &) const override;

private:
  std::vector<muse::audio::AudioDevice> museAvailableDevices() const;
  muse::Inject<IExternalDevicesConfiguration> configuration;
  muse::Inject<muse::audio::IAudioDriver> audioDriver;
  muse::async::Notification m_selectedDeviceChanged;
  bool m_deviceChangeExpected = false;
};
} // namespace dgk