#pragma once

#include <async/channel.h>
#include <draw/types/geometry.h>
#include <modularity/imoduleinterface.h>
#include <notation/notationtypes.h>

namespace dgk
{
class IOrchestrionNotationInteraction : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionNotationInteraction);

public:
  virtual ~IOrchestrionNotationInteraction() = default;

  virtual void onMousePressed(const muse::PointF &logicalPosition,
                              float hitWidth) = 0;
  virtual muse::async::Channel<const mu::notation::EngravingItem *>
  elementClicked() const = 0;
};
} // namespace dgk