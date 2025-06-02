#include "ExternalDeviceSelectionRestorer.h"
#include "IExternalDeviceService.h"

namespace dgk
{
void ExternalDeviceSelectionRestorer::init(IExternalDeviceService &service)
{
  service.selectedDeviceChanged().onNotify(
      this,
      [&]
      {
        if (m_allowLastSelectedDeviceUpdate)
          m_lastSelectedDevice = service.selectedDevice();
        else
          m_allowLastSelectedDeviceUpdate = true;
      });

  service.availableDevicesChanged().onNotify(
      this,
      [&]
      {
        if (!m_lastSelectedDevice)
          return;

        const auto available = service.isAvailable(*m_lastSelectedDevice);
        if (available && service.selectedDevice() == m_lastSelectedDevice)
          return;

        if (available)
          service.selectDevice(m_lastSelectedDevice);
        else
        {
          // Device selection implementation isn't always synchronous ;
          // This flagging normally works, though, and if it very occasionally
          // doesn't it's not the end of the world.
          m_allowLastSelectedDeviceUpdate = false;
          service.selectDefaultDevice();
        }
      });
}
} // namespace dgk