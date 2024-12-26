#include "OrchestrionNotationInteraction.h"
#include <engraving/dom/mscore.h>
#include <notation/notationtypes.h>

namespace dgk::orchestrion
{
void OrchestrionNotationInteraction::onMousePressed(
    const muse::PointF &logicPos, float hitWidth)
{
  const auto interaction = globalContext()->currentNotation()->interaction();
  mu::notation::EngravingItem *hitElement =
      interaction->hitElement(logicPos, hitWidth);
  if (!hitElement)
  {
    interaction->deleteSelection();
    return;
  }

  const auto hitStaff = interaction->hitStaff(logicPos);
  const auto hitStaffIndex = hitStaff ? hitStaff->idx() : muse::nidx;
  interaction->select({hitElement}, mu::engraving::SelectType::REPLACE,
                      hitStaffIndex);

  m_elementClicked.send(hitElement);
}

muse::async::Channel<const mu::notation::EngravingItem *>
OrchestrionNotationInteraction::elementClicked() const
{
  return m_elementClicked;
}
} // namespace dgk::orchestrion
