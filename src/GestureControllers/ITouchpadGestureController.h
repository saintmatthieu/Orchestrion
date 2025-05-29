#pragma once

#include "IGestureController.h"

namespace dgk
{
class ITouchpadGestureController : public IGestureController
{
public:
  virtual ~ITouchpadGestureController() = default;
  virtual muse::async::Channel<Contacts> contactChanged() const = 0;
};
} // namespace dgk