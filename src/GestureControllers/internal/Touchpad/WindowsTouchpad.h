#pragma once

#include "../GestureControllerInternalTypes.h"
#include "IOperatingSystemTouchpad.h"

#include <QAbstractNativeEventFilter>

#include <Windows.h>
#include <hidsdi.h>

#include <functional>
#include <vector>

namespace dgk
{
class WindowsTouchpad : public IOperatingSystemTouchpad,
                        public QAbstractNativeEventFilter
{
public:
  WindowsTouchpad(std::function<void(const TouchpadScan &)> cb);
  ~WindowsTouchpad() override;

  bool isAvailable() const override;

private:
  bool nativeEventFilter(const QByteArray &eventType, void *message,
                         qintptr *result) override;
  bool registerTouchpad(HWND hwnd);
  void onInput(LPARAM param);

  // Informative doc to understand this whole thing:
  // https://docs.microsoft.com/en-us/windows-hardware/design/component-guidelines/supporting-usages-in-multitouch-digitizer-drivers
  struct HidDeviceInfo
  {
    std::vector<USHORT> linkColIDs;
    PHIDP_PREPARSED_DATA preparsedData = nullptr;
    UINT cbPreparsedData = 0u;
    USHORT contactCountLinkCollection = 0u;
    USHORT scanTimeLinkCollection = 0u;
    float xMax = 0.f;
    float yMax = 0.f;
  };

  static HidDeviceInfo getDeviceInfo();

  const std::function<void(const TouchpadScan &)> m_cb;
  const HidDeviceInfo m_deviceInfo;
  std::optional<ULONG> m_prevX;
  std::optional<ULONG> m_prevY;

  bool m_touchpadRegistered = false;
};
} // namespace dgk
