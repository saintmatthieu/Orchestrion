#pragma once

#include "IOrchestrionNotationINteraction.h"
#include <context/iglobalcontext.h>
#include <modularity/ioc.h>

namespace dgk
{
class OrchestrionNotationInteraction : public IOrchestrionNotationInteraction,
                                       public muse::Injectable
{
  muse::Inject<mu::context::IGlobalContext> globalContext = {this};

public:
  void onMousePressed(const muse::PointF &logicalPosition,
                      float hitWidth) override;
  void onMouseMoved() override;
  muse::async::Channel<const mu::notation::Note *> noteClicked() const override;

private:
  mu::notation::INotationInteractionPtr muInteraction() const;
  muse::async::Channel<const mu::notation::Note *> m_noteClicked;
};
} // namespace dgk