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
#include "GestureControllers/ITouchpadGestureController.h"
#include "OrchestrionSequencer/IChord.h"
#include "OrchestrionSequencer/IOrchestrionSequencer.h"
#include "OrchestrionSequencer/IRest.h"
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

void OrchestrionNotationPaintView::onLoadNotation(
    mu::notation::INotationPtr notation)
{
  mu::notation::NotationPaintView::onLoadNotation(std::move(notation));
  // We want hover events, which NotationPaintView::onLoadNotation may have set
  // to false.
  setAcceptHoverEvents(true);
}

bool OrchestrionNotationPaintView::eventFilter(QObject *watched, QEvent *event)
{
  const auto ans = acceptHoverEvents();

  if (watched == this)
  {
    if (event->type() == QEvent::MouseButtonPress)
    {
      const auto mouseEvent = static_cast<QMouseEvent *>(event);
      onMousePressed(mouseEvent->position());
    }
    else if (event->type() == QEvent::HoverMove)
    {
      const auto mouseEvent = static_cast<QMouseEvent *>(event);
      onMouseMoved(mouseEvent->position());
    }
  }

  return mu::notation::NotationPaintView::eventFilter(watched, event);
}

void OrchestrionNotationPaintView::onMousePressed(const QPointF &pos)
{
  const muse::PointF logicPos = toLogical(pos);
  interactionProcessor()->onMousePressed(logicPos, hitWidth());
}

void OrchestrionNotationPaintView::onMouseMoved(const QPointF &pos)
{
  const muse::PointF logicPos = toLogical(pos);
  interactionProcessor()->onMouseMoved(logicPos, hitWidth());
}

float OrchestrionNotationPaintView::hitWidth() const
{
  return configuration()->selectionProximity() * 0.5f / currentScaling();
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

  gestureControllerSelector()->touchpadControllerChanged().onNotify(
      this, [this] { initTouchpadMidiController(); });
  initTouchpadMidiController();

  load();
  updateNotation();

  const auto interaction = notationInteraction();
  IF_ASSERT_FAILED(interaction) { return; }
  interaction->noteInput()->stateChanged().onNotify(
      this,
      [this]
      {
        QTimer::singleShot(0, this,
                           [this]
                           {
                             // Same as above: restore this to `true` in case
                             // the base class has set it to `false`.
                             setAcceptHoverEvents(true);
                           });
      },
      AsyncMode::AsyncSetRepeat);
}

void OrchestrionNotationPaintView::initTouchpadMidiController()
{
  const auto touchpad = gestureControllerSelector()->getTouchpadController();

  // set cursor to invisible if touchpad is not null:
  if (touchpad)
    setCursor(Qt::BlankCursor);
  else
    setCursor(Qt::ArrowCursor);

  if (!touchpad)
    return;
  touchpad->contactChanged().onReceive(
      this,
      [this](const Contacts &contacts)
      {
        // Delete contacts that are no longer active:
        for (auto it = m_contacts.begin(); it != m_contacts.end();)
        {
          if (std::find_if(
                  contacts.begin(), contacts.end(), [&](const auto &entry)
                  { return entry.uid == it->first; }) == contacts.end())
            it = m_contacts.erase(it);
          else
            ++it;
        }

        for (const auto &contact : contacts)
        {
          if (m_contacts.find(contact.uid) == m_contacts.end())
            m_contacts.emplace(contact.uid,
                               Contact{contact.x < 0.5, contact.x, contact.y});
          else
          {
            m_contacts.at(contact.uid).x = contact.x;
            m_contacts.at(contact.uid).y = contact.y;
          }
        }

        update();
      });
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

  const auto view = viewport();

  const auto radius = 30. / currentScaling();

  // Draw a rectangle of the viewport with a vertical line in its middle:
  painter->setPen(Qt::black);
  painter->drawRect(view.toQRectF());
  painter->drawLine(view.left() + view.width() / 2, view.top(),
                    view.left() + view.width() / 2, view.bottom());

  painter->setPen(Qt::NoPen);
  painter->setOpacity(0.1);

  std::for_each(m_contacts.begin(), m_contacts.end(),
                [&](const std::pair<int, Contact> &entry)
                {
                  const Contact &contact = entry.second;
                  painter->setBrush(contact.isLeft ? Qt::red : Qt::blue);
                  // Opacity 0.5
                  // Draw a circle, mapping normalized x and y to the viewport
                  // (physical coordinates)
                  const auto x = view.left() + contact.x * view.width();
                  const auto y = view.top() + contact.y * view.height();
                  painter->drawEllipse(QPointF{x, y}, radius, radius);
                });
}
} // namespace dgk