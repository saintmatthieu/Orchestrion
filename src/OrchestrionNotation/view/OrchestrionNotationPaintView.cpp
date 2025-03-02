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
#include "Orchestrion/IChord.h"
#include "Orchestrion/IOrchestrionSequencer.h"
#include "Orchestrion/IRest.h"
#include <QApplication>
#include <QPainter>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/tie.h>

namespace dgk
{
OrchestrionNotationPaintView::OrchestrionNotationPaintView(QQuickItem *parent)
    : mu::notation::NotationPaintView(parent)
{
}

void OrchestrionNotationPaintView::subscribe(
    const IOrchestrionSequencer &sequencer)
{
  sequencer.ChordTransitions().onReceive(
      this,
      [this](std::map<TrackIndex, ChordTransition> transitions)
      {
        OnTransitions(transitions);
        update();
      });

  if (const auto &transitions = sequencer.GetCurrentTransitions();
      !transitions.empty())
    OnTransitions(transitions);

  sequencer.AboutToJumpPosition().onNotify(this,
                                           [this]
                                           {
                                             m_boxes.clear();
                                             update();
                                           });
}

void OrchestrionNotationPaintView::OnTransitions(
    const std::map<TrackIndex, ChordTransition> &transitions)
{
  for (const auto &[track, transition] : transitions)
  {
    if (GetPastChord(transition))
      m_boxes.erase(track.value);

    const IMelodySegment *present = GetPresentThing(transition);
    const IChord *future = GetFutureChord(transition);
    const auto thing = present ? present : future;
    if (!thing)
      continue;

    const auto segment = chordRegistry()->GetSegment(thing);
    if (!segment)
      // Could be a voice blank, which isn't represented by an engraving segment
      continue;
    const std::vector<mu::engraving::EngravingItem *> items =
        getRelevantItems(track, segment);
    const auto invisible = std::all_of(
        items.begin(), items.end(),
        [](const auto item)
        {
          if (const auto rest = dynamic_cast<mu::engraving::Rest *>(item))
            return rest->isGap();
          else
            return false;
        });
    if (invisible)
      continue;
    mu::engraving::RectF huggingBox;
    for (const auto item : items)
      huggingBox = huggingBox.united(item->pageBoundingRect());
    const auto huggingRect = huggingBox.toQRectF();
    const auto spatium = items.front()->spatium();
    Box &box = m_boxes[track.value];
    box.rect = huggingRect.adjusted(-spatium, -spatium, spatium, spatium);
    box.active = present != nullptr;
    box.opacity = box.active ? 0.9 : 0.4;
    box.pen = QPen{Qt::darkCyan, 10, box.active ? Qt::SolidLine : Qt::DotLine};
  }
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
    onMousePressed(mouseEvent->position());
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

  globalContext()->currentNotationChanged().onNotify(
      this,
      [this]
      {
        AbstractNotationPaintView::onNotationSetup();
        updateNotation();
      });

  load();
  updateNotation();
}

void OrchestrionNotationPaintView::updateNotation()
{
  if (globalContext()->currentNotation())
  {
    setViewMode(mu::notation::ViewMode::LINE);
    alignVertically();
  }
  m_boxes.clear();
  update();
}

void OrchestrionNotationPaintView::setViewMode(mu::notation::ViewMode mode)
{
  const auto notation = this->notation();
  if (!notation)
    return;
  notation->viewState()->setViewMode(mode);
  notation->painting()->setViewMode(mode);
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
  painter->setBrush(Qt::NoBrush);

  std::for_each(m_boxes.begin(), m_boxes.end(),
                [painter](const auto &entry)
                {
                  const Box &box = entry.second;
                  const QRectF &rect = box.rect;
                  painter->setPen(box.pen);
                  painter->setOpacity(box.opacity);
                  painter->drawRoundedRect(rect, rect.width() * .45,
                                           rect.height() * .45);
                });
}
} // namespace dgk