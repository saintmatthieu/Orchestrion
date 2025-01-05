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
#include "OrchestrionNotationPaintView.h"
#include "Orchestrion/IOrchestrionSequencer.h"
#include <QApplication>
#include <QPainter>
#include <engraving/dom/tie.h>

namespace dgk
{
OrchestrionNotationPaintView::OrchestrionNotationPaintView(QQuickItem *parent)
    : mu::notation::NotationPaintView(parent)
{
  qApp->installEventFilter(this);

  orchestrion()->sequencerChanged().onNotify(this,
                                             [this]
                                             {
                                               const auto sequencer =
                                                   orchestrion()->sequencer();
                                               if (!sequencer)
                                                 return;
                                               subscribe(*sequencer);
                                             });
  if (const auto sequencer = orchestrion()->sequencer())
    subscribe(*sequencer);
}

void OrchestrionNotationPaintView::subscribe(
    const IOrchestrionSequencer &sequencer)
{
  sequencer.ChordActivationChanged().onReceive(
      this,
      [this](TrackIndex track, ChordActivationChange change)
      {
        if (!change.deactivated.empty())
          m_boxes.erase(track.value);
        if (!change.activated)
          return;

        const auto segment = chordRegistry()->GetSegment(change.activated);
        IF_ASSERT_FAILED(segment) { return; }
        const std::vector<mu::engraving::EngravingItem *> items =
            getRelevantItems(track, segment);
        mu::engraving::RectF box;
        for (const auto item : items)
          box = box.united(item->pageBoundingRect());
        auto rect = box.toQRectF();
        const auto spatium = items.front()->spatium();
        m_boxes[track.value] =
            rect.adjusted(-spatium, -spatium, spatium, spatium);
      });
}

std::vector<mu::engraving::EngravingItem *>
OrchestrionNotationPaintView::getRelevantItems(
    TrackIndex track, const mu::engraving::Segment *segment) const
{
  const auto chord =
      dynamic_cast<const mu::engraving::Chord *>(segment->element(track.value));
  using NoteVector = std::vector<mu::engraving::Note *>;
  const NoteVector notes = chord ? chord->notes() : NoteVector{};
  std::vector<mu::engraving::EngravingItem *> items;
  std::for_each(notes.begin(), notes.end(),
                [&](mu::engraving::Note *note)
                {
                  while (note)
                  {
                    items.emplace_back(note);
                    auto tie = note->tieFor();
                    if (tie)
                      items.emplace_back(tie);
                    note = tie ? tie->endNote() : nullptr;
                  }
                });

  if (notes.empty())
  {
    // Get all consecutive rests, ignoring elements other than chords such as
    // bars, clefs, etc.
    while (segment)
    {
      auto item = segment->element(track.value);
      if (dynamic_cast<mu::engraving::Chord *>(item))
        break;
      if (const auto rest = dynamic_cast<mu::engraving::Rest *>(item))
        items.emplace_back(rest);
      segment = segment->next();
    }
  }
  return items;
}

bool OrchestrionNotationPaintView::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() == QEvent::MouseButtonPress)
  {
    const auto mouseEvent = static_cast<QMouseEvent *>(event);
    onMousePressed(mouseEvent->localPos());
  }
  else if (event->type() == QEvent::MouseMove)
    orchestrionNotationInteraction()->onMouseMoved();

  return mu::notation::NotationPaintView::eventFilter(watched, event);
}

void OrchestrionNotationPaintView::onMousePressed(const QPointF &pos)
{
  const muse::PointF logicPos = toLogical(pos);
  const float hitWidth =
      configuration()->selectionProximity() * 0.5f / currentScaling();
  orchestrionNotationInteraction()->onMousePressed(logicPos, hitWidth);
}

void OrchestrionNotationPaintView::loadOrchestrionNotation()
{
  load();
  setViewMode(mu::notation::ViewMode::LINE);
  alignVertically();
}

void OrchestrionNotationPaintView::setViewMode(mu::notation::ViewMode mode)
{
  notation()->viewState()->setViewMode(mode);
  notation()->painting()->setViewMode(mode);
}

void OrchestrionNotationPaintView::alignVertically()
{
  const auto canvasRect = fromLogical(notationContentRect());
  const auto y = (height() - canvasRect.height()) / 2.;
  // Don't know why we need to negate.
  moveCanvasToPosition(toLogical(muse::PointF{-y, -y}));
}

void OrchestrionNotationPaintView::paint(QPainter *painter)
{
  NotationPaintView::paint(painter);

  painter->restore();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(QPen(QColorConstants::DarkCyan, 10));
  painter->setBrush(Qt::NoBrush);

  std::for_each(m_boxes.begin(), m_boxes.end(),
                [painter](const auto &entry)
                {
                  const auto &box = entry.second;
                  painter->drawRoundedRect(box, box.width() * .45,
                                           box.height() * .45);
                });
}
} // namespace dgk