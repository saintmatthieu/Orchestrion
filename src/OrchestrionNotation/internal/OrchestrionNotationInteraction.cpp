#include "OrchestrionNotationInteraction.h"
#include <engraving/dom/mscore.h>
#include <notation/notationtypes.h>

namespace dgk
{
void OrchestrionNotationInteraction::onMousePressed(
    const muse::PointF &logicPos, float hitWidth)
{
  const auto interaction = muInteraction();
  mu::notation::EngravingItem *hitElement =
      interaction->hitElement(logicPos, hitWidth);
  const auto note = dynamic_cast<mu::engraving::Note *>(hitElement);
  if (!note)
    // interaction->clearSelection(); using this might be needed.
    return;

  const auto hitStaff = interaction->hitStaff(logicPos);
  const auto hitStaffIndex = hitStaff ? hitStaff->idx() : muse::nidx;
  interaction->select({note}, mu::engraving::SelectType::REPLACE,
                      hitStaffIndex);

  m_noteClicked.send(note);
}

void OrchestrionNotationInteraction::onMouseMoved()
{
  // This will prevent dragging from editing the score.
  muInteraction()->clearSelection();
}

muse::async::Channel<const mu::notation::Note *>
OrchestrionNotationInteraction::noteClicked() const
{
  return m_noteClicked;
}

mu::notation::INotationInteractionPtr
OrchestrionNotationInteraction::muInteraction() const
{
  return globalContext()->currentNotation()->interaction();
}
} // namespace dgk
