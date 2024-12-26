#pragma once

#include <modularity/imoduleinterface.h>
#include <async/notification.h>

namespace dgk
{
class IOrchestrionSequencer;

class IOrchestrion : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrion);

public:
  virtual ~IOrchestrion() = default;

  virtual IOrchestrionSequencer* sequencer() = 0;
  virtual muse::async::Notification sequencerChanged() const = 0;
};
} // namespace dgk