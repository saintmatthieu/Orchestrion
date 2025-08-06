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
#include "OrchestrionNotationInteractionProcessor.h"
#include <engraving/dom/mscore.h>
#include <notation/notationtypes.h>

namespace dgk
{
void OrchestrionNotationInteractionProcessor::onMousePressed(
    const muse::PointF &logicPos, float hitWidth)
{
  const auto interaction = muInteraction();
  if (!interaction)
    return;
  mu::notation::EngravingItem *hitElement =
      interaction->hitElement(logicPos, hitWidth);
  if (hitElement)
    m_itemClicked.send(hitElement);
}

void OrchestrionNotationInteractionProcessor::onMouseMoved()
{
  // This will prevent dragging from editing the score.
  if (const auto interaction = muInteraction())
    interaction->clearSelection();
}

muse::async::Channel<const mu::notation::EngravingItem *>
OrchestrionNotationInteractionProcessor::itemClicked() const
{
  return m_itemClicked;
}

mu::notation::INotationInteractionPtr
OrchestrionNotationInteractionProcessor::muInteraction() const
{
  const auto notation = globalContext()->currentNotation();
  return notation ? notation->interaction() : nullptr;
}
} // namespace dgk
