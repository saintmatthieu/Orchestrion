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

#include <QApplication>

namespace dgk
{
void OrchestrionNotationInteractionProcessor::onMousePressed(
    const muse::PointF &logicPos, float hitWidth)
{
  const auto interaction = muInteraction();
  if (!interaction)
    return;
  const mu::notation::EngravingItem *const hitElement =
      interaction->hitElement(logicPos, hitWidth);
  if (hitElement)
    m_itemClicked.send(hitElement);
}

void OrchestrionNotationInteractionProcessor::onMouseMoved(
    const muse::PointF &logicPos, float hitWidth)
{
  const auto interaction = muInteraction();
  if (!interaction)
    return;

  const mu::notation::EngravingItem *const hitElement =
      interaction->hitElement(logicPos, hitWidth);
  if (hitElement)
  {
    if (!m_pointingHandCursor)
    {
      QApplication::setOverrideCursor(Qt::PointingHandCursor);
      m_pointingHandCursor = true;
    }
  }
  else if (m_pointingHandCursor)
  {
    QApplication::restoreOverrideCursor();
    m_pointingHandCursor = false;
  }

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
