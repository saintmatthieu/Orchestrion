#pragma once

#include "IOrchestrionSequencer.h"
#include <async/notification.h>
#include <modularity/imoduleinterface.h>

namespace dgk
{
class IOrchestrionSequencer;

class IOrchestrion : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrion);

public:
  virtual ~IOrchestrion() = default;

  virtual IOrchestrionSequencerPtr sequencer() = 0;
  virtual muse::async::Notification sequencerChanged() const = 0;
};
} // namespace dgk