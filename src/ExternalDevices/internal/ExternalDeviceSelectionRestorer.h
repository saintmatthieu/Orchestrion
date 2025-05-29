#pragma once

#include "ExternalDevicesTypes.h"

#include "async/asyncable.h"
#include "modularity/ioc.h"

namespace dgk
{
class IExternalDeviceService;

/*!
 * \brief Automates what a user would do after the device she had selected went
 * disconnected and reconnected.
 */
class ExternalDeviceSelectionRestorer : public muse::async::Asyncable,
                                        public muse::Injectable
{
public:
  void init(IExternalDeviceService &);

private:
  std::optional<ExternalDeviceId> m_lastSelectedDevice;
};
} // namespace dgk