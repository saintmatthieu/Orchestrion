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
#include "OrchestrionSequencer/NoteLabel.h"
#include <engraving/dom/factory.h>
#include <engraving/dom/fingering.h>
#include <engraving/dom/mscore.h>
#include <engraving/dom/note.h>
#include <engraving/dom/score.h>
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

const mu::notation::EngravingItem *
OrchestrionNotationInteractionProcessor::hitNoteAt(const muse::PointF &logicPos,
                                                   float hitWidth) const
{
  const auto interaction = muInteraction();
  if (!interaction)
    return nullptr;
  const mu::notation::EngravingItem *const hit =
      interaction->hitElement(logicPos, hitWidth);
  return hit && hit->isNote() ? hit : nullptr;
}

QString OrchestrionNotationInteractionProcessor::noteLabel(
    const mu::notation::EngravingItem *item) const
{
  if (!item || !item->isNote())
    return {};
  const mu::engraving::Note *const note = mu::engraving::toNote(item);
  for (const mu::engraving::EngravingItem *e : note->el())
    if (e->type() == mu::engraving::ElementType::FINGERING)
    {
      const QString label = noteLabelFromFingering(
          mu::engraving::toFingering(e)->plainText().toQString());
      if (!label.isEmpty())
        return label;
    }
  return {};
}

void OrchestrionNotationInteractionProcessor::setNoteLabel(
    const mu::notation::EngravingItem *item, const QString &text)
{
  if (!item || !item->isNote())
    return;
  mu::engraving::Note *const note = mu::engraving::toNote(
      const_cast<mu::engraving::EngravingItem *>(item));
  mu::engraving::Score *const score = note->score();
  if (!score)
    return;

  // Replace only a label we previously added (marker-tagged); leave ordinary
  // fingerings already on the note untouched.
  mu::engraving::EngravingItem *existing = nullptr;
  for (mu::engraving::EngravingItem *e : note->el())
    if (e->type() == mu::engraving::ElementType::FINGERING &&
        !noteLabelFromFingering(
             mu::engraving::toFingering(e)->plainText().toQString())
             .isEmpty())
    {
      existing = e;
      break;
    }

  const QString trimmed = text.trimmed();

  score->startCmd();
  if (existing)
    score->undoRemoveElement(existing);
  if (!trimmed.isEmpty())
  {
    mu::engraving::Fingering *const fingering =
        mu::engraving::Factory::createFingering(note);
    fingering->setTrack(note->track());
    fingering->setParent(note);
    fingering->setPlainText(
        muse::String::fromQString(trimmed + noteLabelMarker()));
    score->undoAddElement(fingering);
  }
  score->endCmd();
}
} // namespace dgk
