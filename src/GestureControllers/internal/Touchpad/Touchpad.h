#pragma once

#include "ITouchpad.h"
#include "OperatingSystemTouchpadProcessor.h"
#include "IOperatingSystemTouchpad.h"

namespace dgk
{
class Touchpad : public ITouchpad
{
public:
  Touchpad();

  bool isAvailable() const override;
  muse::async::Channel<Contacts> contactChanged() const override;

private:
  const std::unique_ptr<IOperatingSystemTouchpad> m_osTouchpad;
  OperatingSystemTouchpadProcessor m_processor;
};
} // namespace dgk
