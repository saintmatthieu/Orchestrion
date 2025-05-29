#pragma once

#include "modularity/ioc.h"

namespace dgk
{
class IExternalDevicesConfiguration : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IExternalDevicesConfiguration)

public:
  virtual ~IExternalDevicesConfiguration() = default;

  virtual std::optional<ExternalDeviceId> readSelectedMidiDevice() const = 0;
  virtual std::optional<ExternalDeviceId> readSelectedAudioDevice() const = 0;

  virtual void writeSelectedMidiDevice(const std::optional<ExternalDeviceId> &) = 0;
  virtual void writeSelectedAudioDevice(const std::optional<ExternalDeviceId> &) = 0;
};
} // namespace dgk