#pragma once

#include "internal/IExternalDeviceService.h"

#include "async/notification.h"
#include "modularity/ioc.h"
#include <optional>

namespace dgk
{
class IMidiDeviceService : public IExternalDeviceService,
                           MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IMidiDeviceService);

public:
  virtual ~IMidiDeviceService() = default;
  virtual muse::async::Notification startupSelectionFinished() const = 0;
};
} // namespace dgk