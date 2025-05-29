#include "WindowsTouchpad.h"

#include <global/defer.h>
#include <log.h>

#include <QApplication>
#include <QByteArray>

#include <WinUser.h>
#include <hidusage.h>

// For some reason, these usages are not defined in hidusage.h
#ifndef HID_USAGE_DIGITIZER_CONTACT_IDENTIFIER
#define HID_USAGE_DIGITIZER_CONTACT_IDENTIFIER ((USAGE)0x51)
#endif
#ifndef HID_USAGE_DIGITIZER_CONTACT_COUNT
#define HID_USAGE_DIGITIZER_CONTACT_COUNT ((USAGE)0x54)
#endif
#ifndef HID_USAGE_DIGITIZER_SCAN_TIME
#define HID_USAGE_DIGITIZER_SCAN_TIME ((USAGE)0x55)
#endif
#ifndef HID_USAGE_DIGITIZER_CONTACT_ID
#define HID_USAGE_DIGITIZER_CONTACT_ID ((USAGE)0x51)
#endif

namespace dgk
{
WindowsTouchpad::HidDeviceInfo WindowsTouchpad::getDeviceInfo()
{
  HidDeviceInfo deviceInfo;

  auto numDevices = 0u;
  const auto result =
      GetRawInputDeviceList(nullptr, &numDevices, sizeof(RAWINPUTDEVICELIST));
  if (result == -1)
    return deviceInfo;

  std::vector<RAWINPUTDEVICELIST> deviceList(numDevices);
  GetRawInputDeviceList(deviceList.data(), &numDevices,
                        sizeof(RAWINPUTDEVICELIST));

  // remove all devices where device.dwType != RIM_TYPEHID:
  deviceList.erase(std::remove_if(deviceList.begin(), deviceList.end(),
                                  [](const auto &device)
                                  { return device.dwType != RIM_TYPEHID; }),
                   deviceList.end());

  auto found = false;

  for (const auto &device : deviceList)
  {
    // get preparsed data for HidP
    UINT cbDataSize = 0;
    PHIDP_PREPARSED_DATA preparsedData = NULL;

    if (GetRawInputDeviceInfo(device.hDevice, RIDI_PREPARSEDDATA, NULL,
                              &cbDataSize) == (UINT)-1)
      continue;
    preparsedData = (PHIDP_PREPARSED_DATA)malloc(cbDataSize);
    muse::Defer freePreparsedData{[preparsedData] { free(preparsedData); }};

    if (GetRawInputDeviceInfo(device.hDevice, RIDI_PREPARSEDDATA, preparsedData,
                              &cbDataSize) == (UINT)-1)
      continue;
    HIDP_CAPS caps;
    if (HidP_GetCaps(preparsedData, &caps) != HIDP_STATUS_SUCCESS)
      continue;

    if (caps.NumberInputValueCaps == 0)
      continue;

    const USHORT numValueCaps = caps.NumberInputValueCaps;
    USHORT _numValueCaps = numValueCaps;

    PHIDP_VALUE_CAPS valueCaps =
        (PHIDP_VALUE_CAPS)malloc(sizeof(HIDP_VALUE_CAPS) * numValueCaps);
    muse::Defer freeValueCaps{[valueCaps] { free(valueCaps); }};

    if (HidP_GetValueCaps(HidP_Input, valueCaps, &_numValueCaps,
                          preparsedData) != HIDP_STATUS_SUCCESS)
      continue;

    for (USHORT valueCapIndex = 0; valueCapIndex < numValueCaps;
         valueCapIndex++)
    {
      const HIDP_VALUE_CAPS &cap = valueCaps[valueCapIndex];

      if (cap.IsRange || !cap.IsAbsolute)
        continue;

      LOGI() << "UsagePage: " << cap.UsagePage;
      if (cap.UsagePage != HID_USAGE_PAGE_GENERIC &&
          cap.UsagePage != HID_USAGE_PAGE_DIGITIZER)
        continue;

      if (cap.UsagePage == HID_USAGE_PAGE_GENERIC)
      {
        std::optional<USHORT> linkColID;
        const auto it =
            std::find(deviceInfo.linkColIDs.begin(),
                      deviceInfo.linkColIDs.end(), cap.LinkCollection);
        if (it != deviceInfo.linkColIDs.end())
        {
          linkColID.emplace(*it);
          deviceInfo.linkColIDs.erase(it);
        }

        if (cap.NotRange.Usage == HID_USAGE_GENERIC_X)
        {
          if (!linkColID.has_value())
            linkColID = cap.LinkCollection;

          deviceInfo.xMax = cap.LogicalMax;
          linkColID = cap.LinkCollection;
        }
        else if (cap.NotRange.Usage == HID_USAGE_GENERIC_Y)
        {
          if (!linkColID.has_value())
            linkColID = cap.LinkCollection;

          deviceInfo.yMax = cap.LogicalMax;
          linkColID = cap.LinkCollection;
        }

        if (linkColID.has_value())
          deviceInfo.linkColIDs.push_back(*linkColID);
      }
      else
      {
        switch (cap.NotRange.Usage)
        {
        case HID_USAGE_DIGITIZER_CONTACT_IDENTIFIER:
          deviceInfo.scanTimeLinkCollection = cap.LinkCollection;
          break;
        case HID_USAGE_DIGITIZER_CONTACT_COUNT:
          deviceInfo.contactCountLinkCollection = cap.LinkCollection;
          break;
        }
      }

      found = true;
    }

    if (found)
    {
      deviceInfo.preparsedData = (PHIDP_PREPARSED_DATA)malloc(cbDataSize);
      memcpy(deviceInfo.preparsedData, preparsedData, cbDataSize);
      deviceInfo.cbPreparsedData = cbDataSize;
      break;
    }
  }

  assert(found);
  return deviceInfo;
}

WindowsTouchpad::WindowsTouchpad(std::function<void(const TouchpadScan &)> cb)
    : m_cb{std::move(cb)}, m_deviceInfo{getDeviceInfo()}
{
  if (isAvailable())
    qApp->installNativeEventFilter(this);
}

WindowsTouchpad::~WindowsTouchpad()
{
  if (m_deviceInfo.preparsedData != nullptr)
    free(m_deviceInfo.preparsedData);
}

bool WindowsTouchpad::isAvailable() const
{
  return m_deviceInfo.preparsedData != nullptr;
}

bool WindowsTouchpad::nativeEventFilter(const QByteArray &eventType,
                                        void *message, qintptr *)
{
  if (eventType != "windows_generic_MSG")
    return false;

  MSG *msg = static_cast<MSG *>(message);

  if (msg->message == WM_INPUT)
    onInput(msg->lParam);
  else if (!m_touchpadRegistered)
  {
    if (!registerTouchpad(msg->hwnd))
    {
      LOGE() << "Failed to register touchpad!";
    }
    else
      m_touchpadRegistered = true;
  }
  return false;
}

bool WindowsTouchpad::registerTouchpad(HWND hwnd)
{
  // register Windows Precision WindowsTouchpad top-level HID collection

  RAWINPUTDEVICE rid;

  rid.usUsagePage = HID_USAGE_PAGE_DIGITIZER;
  rid.usUsage = HID_USAGE_DIGITIZER_TOUCH_PAD;
  rid.dwFlags = NULL;
  rid.hwndTarget = hwnd;

  return RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));
}

namespace
{
bool Ok(int hidpReturnCode)
{
  if (hidpReturnCode == HIDP_STATUS_SUCCESS)
    return true;

  // The comments on the errors are copied from
  // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/hidpi/nf-hidpi-hidp_getusagevalue
  switch (hidpReturnCode)
  {
  case HIDP_STATUS_INCOMPATIBLE_REPORT_ID:
    // The collection contains a value on the specified usage page in a report
    // of the specified type, but there are no such usages in the specified
    // report.
    LOGE() << "HIDP_STATUS_INCOMPATIBLE_REPORT_ID";
    break;
  case HIDP_STATUS_INVALID_PREPARSED_DATA:
    // The preparsed data is not valid.
    LOGE() << "HIDP_STATUS_INVALID_PREPARSED_DATA";
    break;
  case HIDP_STATUS_USAGE_NOT_FOUND:
    // The collection does not contain a value on the specified usage page in
    // any report of the specified report type.
    LOGE() << "HIDP_STATUS_USAGE_NOT_FOUND";
    break;
  default:
    LOGE() << "Unknown error";
  }
  return false;
}
} // namespace

void WindowsTouchpad::onInput(LPARAM param)
{
  UINT rawInputSize;
  PRAWINPUT rawInputData = nullptr;
  GetRawInputData(reinterpret_cast<HRAWINPUT>(param), RID_INPUT, nullptr,
                  &rawInputSize, sizeof(RAWINPUTHEADER));
  rawInputData = static_cast<PRAWINPUT>(malloc(rawInputSize));
  muse::Defer freeRawInputData{[rawInputData] { free(rawInputData); }};
  GetRawInputData(reinterpret_cast<HRAWINPUT>(param), RID_INPUT, rawInputData,
                  &rawInputSize, sizeof(RAWINPUTHEADER));

  if (rawInputData->header.dwType != RIM_TYPEHID ||
      rawInputData->data.hid.dwCount == 0)
    return;

  const auto touchpadData = rawInputData->data.hid.bRawData;
  const auto touchpadDataSize = rawInputData->data.hid.dwSizeHid;

  ULONG usageValue;

  // Get the number of contacts
  if (!Ok(HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_DIGITIZER, 0,
                             HID_USAGE_DIGITIZER_CONTACT_COUNT, &usageValue,
                             m_deviceInfo.preparsedData, (PCHAR)touchpadData,
                             touchpadDataSize)))
  {
    LOGE() << "Failed to read number of contacts!";
    return;
  }

  // Get scan time:
  ULONG scanTimeValue;
  if (!Ok(HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_DIGITIZER, 0,
                             HID_USAGE_DIGITIZER_SCAN_TIME, &scanTimeValue,
                             m_deviceInfo.preparsedData, (PCHAR)touchpadData,
                             touchpadDataSize)))
  {
    LOGE() << "Failed to read scan time!";
    return;
  }

  assert(usageValue <= m_deviceInfo.linkColIDs.size());
  const auto numContacts =
      std::min<size_t>(usageValue, m_deviceInfo.linkColIDs.size());

  static int64_t prevScanTime = 0;
  while (scanTimeValue < prevScanTime)
    scanTimeValue += 1 << 16;
  prevScanTime = scanTimeValue;

  TouchpadScan scan{std::chrono::milliseconds{scanTimeValue / 10}};

  for (auto i = 0u; i < numContacts; ++i)
  {
    const auto id = m_deviceInfo.linkColIDs[i];
    ULONG xPos = 0, yPos = 0, contactID = 0;

    if (!Ok(HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, id,
                               HID_USAGE_GENERIC_X, &xPos,
                               m_deviceInfo.preparsedData, (PCHAR)touchpadData,
                               touchpadDataSize)) ||
        !Ok(HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_GENERIC, id,
                               HID_USAGE_GENERIC_Y, &yPos,
                               m_deviceInfo.preparsedData, (PCHAR)touchpadData,
                               touchpadDataSize)) ||
        !Ok(HidP_GetUsageValue(HidP_Input, HID_USAGE_PAGE_DIGITIZER, id,
                               HID_USAGE_DIGITIZER_CONTACT_ID, &contactID,
                               m_deviceInfo.preparsedData, (PCHAR)touchpadData,
                               touchpadDataSize)))
    {
      assert(false);
      continue;
    }

    m_prevX = xPos;
    m_prevY = yPos;
    scan.contacts.emplace_back(TouchpadContact{static_cast<int>(contactID),
                                               xPos / m_deviceInfo.xMax,
                                               yPos / m_deviceInfo.yMax});
  }

  m_cb(std::move(scan));
}
} // namespace dgk