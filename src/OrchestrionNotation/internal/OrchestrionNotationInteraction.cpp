/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "OrchestrionNotationInteraction.h"
#include <engraving/dom/mscore.h>
#include <notation/notationtypes.h>

namespace dgk
{
void OrchestrionNotationInteraction::onMousePressed(
    const muse::PointF &logicPos, float hitWidth)
{
  const auto interaction = muInteraction();
  if (!interaction)
    return;
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
  if (const auto interaction = muInteraction())
    interaction->clearSelection();
}

muse::async::Channel<const mu::notation::Note *>
OrchestrionNotationInteraction::noteClicked() const
{
  return m_noteClicked;
}

mu::notation::INotationInteractionPtr
OrchestrionNotationInteraction::muInteraction() const
{
  const auto notation = globalContext()->currentNotation();
  return notation ? notation->interaction() : nullptr;
}
} // namespace dgk
