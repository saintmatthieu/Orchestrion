#pragma once

#include "ExternalDevicesTypes.h"
#include "internal/IExternalDeviceService.h"

#include "async/notification.h"
#include "modularity/ioc.h"
#include <optional>

namespace dgk
{
class IAudioDeviceService : public IExternalDeviceService,
                            MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IAudioDeviceService);

public:
  virtual ~IAudioDeviceService() = default;
};
} // namespace dgk