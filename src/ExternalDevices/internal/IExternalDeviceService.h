#pragma once

#include "../ExternalDevicesTypes.h"

#include "async/notification.h"
#include <optional>
#include <vector>

namespace dgk
{
class IExternalDeviceService
{
public:
  virtual ~IExternalDeviceService() = default;

  // Returns the names of the available devices.
  virtual std::vector<ExternalDeviceId> availableDevices() const = 0;
  virtual muse::async::Notification availableDevicesChanged() const = 0;
  virtual bool isAvailable(const ExternalDeviceId &) const = 0;

  // Does not have to be available.
  virtual void selectDevice(const std::optional<ExternalDeviceId> &) = 0;
  virtual muse::async::Notification selectedDeviceChanged() const = 0;
  virtual std::optional<ExternalDeviceId> selectedDevice() const = 0;

  virtual void selectDefaultDevice() = 0;

  bool selectedDeviceIsAvailable() const
  {
    return selectedDevice() && isAvailable(*selectedDevice());
  }

  virtual std::string deviceName(const ExternalDeviceId &) const = 0;
};
} // namespace dgk