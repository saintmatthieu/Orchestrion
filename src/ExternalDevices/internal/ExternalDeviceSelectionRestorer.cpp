#include "ExternalDeviceSelectionRestorer.h"
#include "IExternalDeviceService.h"

namespace dgk
{
void ExternalDeviceSelectionRestorer::init(IExternalDeviceService &service)
{
  service.selectedDeviceChanged().onNotify(
      this, [&] { m_lastSelectedDevice = service.selectedDevice(); });

  service.availableDevicesChanged().onNotify(
      this,
      [&]
      {
        if (m_lastSelectedDevice &&
            service.selectedDevice() != m_lastSelectedDevice &&
            service.isAvailable(*m_lastSelectedDevice))
          service.selectDevice(m_lastSelectedDevice);
      });
}
} // namespace dgk