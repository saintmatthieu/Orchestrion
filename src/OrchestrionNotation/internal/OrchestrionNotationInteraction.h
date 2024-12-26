#pragma once

#include "IOrchestrionNotationINteraction.h"
#include <context/iglobalcontext.h>
#include <modularity/ioc.h>

namespace dgk::orchestrion
{
class OrchestrionNotationInteraction : public IOrchestrionNotationInteraction,
                                       public muse::Injectable
{
  muse::Inject<mu::context::IGlobalContext> globalContext = {this};

public:
  void onMousePressed(const muse::PointF &logicalPosition,
                      float hitWidth) override;
  muse::async::Channel<const mu::notation::EngravingItem *>
  elementClicked() const override;

private:
  muse::async::Channel<const mu::notation::EngravingItem *> m_elementClicked;
};
} // namespace dgk::orchestrion