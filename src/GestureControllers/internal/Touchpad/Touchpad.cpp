#include "Touchpad.h"
#include "OperatingSystemTouchpadFactory.h"
#include "SteadyClock.h"
#include "MacosTouchpad.h"

#include <log.h>

namespace dgk
{
Touchpad::Touchpad()
    : m_osTouchpad{createOperatingSystemTouchpad(
          [this](const TouchpadScan &scan) { m_processor.process(scan); })},
      m_processor{std::make_unique<SteadyClock>()}
{
  startListeningToTrackpadEvents();
}

bool Touchpad::isAvailable() const { return m_osTouchpad->isAvailable(); }

muse::async::Channel<Contacts> Touchpad::contactChanged() const
{
  return m_processor.contactChanged();
}
} // namespace dgk