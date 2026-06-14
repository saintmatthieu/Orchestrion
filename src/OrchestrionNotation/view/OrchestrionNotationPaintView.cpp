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
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <engraving/dom/chord.h>
#include <engraving/dom/masterscore.h>
#include <engraving/dom/note.h>
#include <engraving/dom/tie.h>

#include <cmath>

namespace dgk
{
OrchestrionNotationPaintView::OrchestrionNotationPaintView(QQuickItem *parent)
    : mu::notation::NotationPaintView(parent),
      m_kineticScroller([this](qreal physicalDx)
                        { return moveCanvasBy(physicalDx); })
{
}

void OrchestrionNotationPaintView::subscribe(
    const IOrchestrionSequencer &sequencer,
    const IModifiableItemRegistry &registry)
{
  sequencer.ChordTransitions().onReceive(
      this,
      [this](std::map<TrackIndex, ChordTransition> transitions)
      {
        OnTransitions(transitions);
        update();
      });

  registry.ModifiedChanged().onNotify(this, [this] { update(); });

  if (const auto &transitions = sequencer.GetCurrentTransitions();
      !transitions.empty())
    OnTransitions(transitions);

  sequencer.AboutToJumpPosition().onReceive(this,
                                            [this](auto)
                                            {
                                              m_kineticScroller.stop();
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
    box.spatium = spatium;
    // Mahogany theme color; a ringing note is highlighted at full strength,
    // the next note sits faintly pre-lit. (Decaying the intensity over the
    // note's ring would need a render timer feeding a ring level here.)
    constexpr auto mahogany = "#5A2B25";

    box.color = QColor(mahogany);
    box.intensity = box.active ? 1.0 : 0.3;
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
  const auto type = event->type();
  if (type == QEvent::HoverMove || type == QEvent::MouseMove ||
      type == QEvent::MouseButtonPress)
  {
    bool inScope = false;
    for (QObject *o = watched; o; o = o->parent())
      if (o == this)
      {
        inScope = true;
        break;
      }

    if (inScope)
    {
      // Gate on real cursor movement: Qt synthesises hover events as the
      // scene graph updates (e.g. when a child runs an infinite opacity
      // animation), and those carry the unchanged cursor position.
      if (type == QEvent::MouseButtonPress)
        emit mouseActivity();
      else
      {
        const QPoint pos = QCursor::pos();
        if (pos != m_lastCursorPos)
        {
          m_lastCursorPos = pos;
          emit mouseActivity();
        }
      }
    }
  }

  if (watched == this)
  {
    const auto mouseEvent = static_cast<QMouseEvent *>(event);
    switch (event->type())
    {
    case QEvent::MouseButtonPress:
      onMousePressed(mouseEvent->position(), mouseEvent->modifiers(),
                     mouseEvent->button());
      break;
    case QEvent::MouseMove:
      onMouseDragged(mouseEvent->position(), mouseEvent->buttons());
      break;
    case QEvent::MouseButtonRelease:
      onMouseReleased(mouseEvent->button());
      break;
    case QEvent::HoverMove:
      onMouseMoved(mouseEvent->position());
      break;
    default:
      break;
    }
  }

  return mu::notation::NotationPaintView::eventFilter(watched, event);
}

void OrchestrionNotationPaintView::onMousePressed(
    const QPointF &pos, Qt::KeyboardModifiers modifiers, Qt::MouseButton button)
{
  m_kineticScroller.stop(); // a click on the score halts an in-progress glide
  const muse::PointF logicPos = toLogical(pos);
  const auto interaction = notationInteraction();
  const bool onElement =
      interaction && interaction->hitElement(logicPos, hitWidth());
  interactionProcessor()->onMousePressed(logicPos, hitWidth());

  // A plain left-drag starting on empty background pans the canvas (the base
  // view does the panning); track it so the release can add a kinetic throw.
  // Pressing an element, or holding a modifier, is selection — not panning.
  m_canvasDragging =
      button == Qt::LeftButton && modifiers == Qt::NoModifier && !onElement;
  if (m_canvasDragging)
  {
    m_lastDragPos = pos;
    m_kineticScroller.beginDrag();
  }
}

void OrchestrionNotationPaintView::onMouseDragged(const QPointF &pos,
                                                  Qt::MouseButtons buttons)
{
  if (!m_canvasDragging || !(buttons & Qt::LeftButton))
    return;
  // The base view pans the canvas to follow the cursor; we only feed the
  // horizontal cursor delta to the scroller so it can throw on release.
  m_kineticScroller.addDragSample(pos.x() - m_lastDragPos.x());
  m_lastDragPos = pos;
}

void OrchestrionNotationPaintView::onMouseReleased(Qt::MouseButton button)
{
  if (button != Qt::LeftButton || !m_canvasDragging)
    return;
  m_canvasDragging = false;
  m_kineticScroller.endDrag();
}

void OrchestrionNotationPaintView::onMouseMoved(const QPointF &pos)
{
  const muse::PointF logicPos = toLogical(pos);
  interactionProcessor()->onMouseMoved(logicPos, hitWidth());

  if (sequencerConfiguration()->noteInfoTooltipEnabled())
    updateHoveredNoteInfo(pos);
  else if (!m_hoveredNoteInfo.isEmpty())
    setHoveredNoteInfo({}, pos);
}

void OrchestrionNotationPaintView::updateHoveredNoteInfo(const QPointF &itemPos)
{
  const auto interaction = notationInteraction();
  if (!interaction)
  {
    setHoveredNoteInfo({}, itemPos);
    return;
  }

  const mu::engraving::EngravingItem *const hitElement =
      interaction->hitElement(toLogical(itemPos), hitWidth());

  const mu::engraving::Chord *chord = nullptr;
  if (const auto note = dynamic_cast<const mu::engraving::Note *>(hitElement))
    chord = note->chord();
  else
    chord = dynamic_cast<const mu::engraving::Chord *>(hitElement);

  if (!chord)
  {
    setHoveredNoteInfo({}, itemPos);
    return;
  }

  // Map the hovered engraving chord back to its MuseChord. Matching on the
  // engraving-chord pointer disambiguates voices/staves sharing a segment.
  const IChord *museChord = nullptr;
  for (IMelodySegment *const segment : chordRegistry()->GetMelodySegments())
    if (IChord *const candidate = segment->AsChord();
        candidate && candidate->GetEngravingChord() == chord)
    {
      museChord = candidate;
      break;
    }

  if (!museChord)
  {
    setHoveredNoteInfo({}, itemPos);
    return;
  }

  const float dynamicVelocity = museChord->GetDynamicVelocity().value_or(0.f);
  setHoveredNoteInfo(
      QStringLiteral("dynamicVelocity: %1").arg(dynamicVelocity, 0, 'f', 3),
      itemPos);
}

void OrchestrionNotationPaintView::setHoveredNoteInfo(const QString &info,
                                                      const QPointF &itemPos)
{
  if (m_hoveredNoteInfo == info && m_hoveredNoteInfoPos == itemPos)
    return;
  m_hoveredNoteInfo = info;
  m_hoveredNoteInfoPos = itemPos;
  emit hoveredNoteInfoChanged();
}

QString OrchestrionNotationPaintView::hoveredNoteInfo() const
{
  return m_hoveredNoteInfo;
}

QPointF OrchestrionNotationPaintView::hoveredNoteInfoPos() const
{
  return m_hoveredNoteInfoPos;
}

float OrchestrionNotationPaintView::hitWidth() const
{
  return configuration()->selectionProximity() * 0.5f / currentScaling();
}

void OrchestrionNotationPaintView::wheelEvent(QWheelEvent *event)
{
  // Ctrl + wheel (or Ctrl + two-finger trackpad swipe, which Qt delivers as a
  // Ctrl-modified wheel event) zooms the score in/out about the cursor.
  if (event->modifiers() & Qt::ControlModifier)
  {
    zoomBy(*event);
    event->accept();
    return;
  }

  // The base class swallows wheel events (zoom + 2D scroll) because the
  // Orchestrion view controls its own scaling and keeps the single LINE-mode
  // system vertically centered. We re-enable just the one gesture we want:
  // a horizontal trackpad swipe pans the viewport left/right (with a kinetic
  // "throw"). Vertical scroll is ignored — there is nothing to scroll there.
  if (m_kineticScroller.handleWheelEvent(*event, width()))
    event->accept();
  else
    event->ignore();
}

void OrchestrionNotationPaintView::zoomBy(const QWheelEvent &event)
{
  // Mirrors mu::notation::NotationViewInputController::wheelEvent: turn the
  // wheel delta into "steps", then scale by zoomSpeed^steps about the cursor.
  // setScaling() runs constrainScorePosition() (via onMatrixChanged), so the
  // single LINE-mode system stays vertically centered after the zoom.
  QPoint pixels = event.pixelDelta();
  const QPoint angle = event.angleDelta();

#ifdef Q_OS_LINUX
  // pixelDelta is unreliable on X11; only trust it under Wayland (same caveat
  // as the base class and the KineticScroller).
  if (qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY"))
    pixels = QPoint{};
#endif

  // A mouse notch is one step; a trackpad reports finer pixel deltas.
  constexpr int pixelsPerStep = 5;
  qreal stepsX = 0.;
  qreal stepsY = 0.;
  if (!pixels.isNull())
  {
    stepsX = pixels.x() / static_cast<qreal>(pixelsPerStep);
    stepsY = pixels.y() / static_cast<qreal>(pixelsPerStep);
  }
  else if (!angle.isNull())
  {
    stepsX = angle.x() / static_cast<qreal>(QWheelEvent::DefaultDeltasPerStep);
    stepsY = angle.y() / static_cast<qreal>(QWheelEvent::DefaultDeltasPerStep);
  }

  const qreal steps = std::sqrt(stepsX * stepsX + stepsY * stepsY) *
                      (stepsY > -stepsX ? 1 : -1);
  if (qFuzzyIsNull(steps))
    return;

  const qreal zoomSpeed =
      std::pow(2.0, 1.0 / configuration()->mouseZoomPrecision());
  qreal scaling = currentScaling() * std::pow(zoomSpeed, steps);

  // Clamp to the zoom range MuseScore allows (its 5%–1600% list).
  const QList<int> zooms = configuration()->possibleZoomPercentageList();
  if (!zooms.isEmpty())
  {
    const qreal minScaling =
        configuration()->scalingFromZoomPercentage(zooms.first());
    const qreal maxScaling =
        configuration()->scalingFromZoomPercentage(zooms.last());
    scaling = std::clamp(scaling, minScaling, maxScaling);
  }

  setScaling(scaling, muse::PointF::fromQPointF(event.position()));
}

bool OrchestrionNotationPaintView::moveCanvasBy(qreal physicalDx)
{
  // The KineticScroller works in physical pixels; convert to the score's
  // logical units. constrainScorePosition() (via onMatrixChanged) clamps the
  // result, so at an edge the viewport doesn't move and we report that back to
  // stop the glide.
  const qreal before = viewport().left();
  moveCanvasHorizontal(physicalDx / currentScaling());
  return qAbs(viewport().left() - before) > 1e-3;
}

void OrchestrionNotationPaintView::loadOrchestrionNotation()
{
  qApp->installEventFilter(this);

  orchestrion()->sequencerChanged().onNotify(
      this,
      [this]
      {
        const auto sequencer = orchestrion()->sequencer();
        const auto registry = orchestrion()->modifiableItemRegistry();
        if (!sequencer || !registry)
          return;
        subscribe(*sequencer, *registry);
      });
  const auto sequencer = orchestrion()->sequencer();
  const auto registry = orchestrion()->modifiableItemRegistry();
  if (sequencer && registry)
    subscribe(*sequencer, *registry);

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

  sequencerConfiguration()->noteInfoTooltipEnabledChanged().onNotify(
      this,
      [this]
      {
        if (!sequencerConfiguration()->noteInfoTooltipEnabled())
          setHoveredNoteInfo({}, m_hoveredNoteInfoPos);
      });

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
          if (std::find_if(contacts.begin(), contacts.end(),
                           [&](const auto &entry) {
                             return entry.uid == it->first;
                           }) == contacts.end())
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

void OrchestrionNotationPaintView::onMatrixChanged(
    const muse::draw::Transform &oldMatrix,
    const muse::draw::Transform &newMatrix, bool overrideZoomType)
{
  NotationPaintView::onMatrixChanged(oldMatrix, newMatrix, overrideZoomType);
  constrainScorePosition();
}

void OrchestrionNotationPaintView::updateNotation()
{
  m_kineticScroller.stop(); // the score changed under us; cancel any glide
  if (const auto notation = globalContext()->currentNotation())
  {
    setViewMode(mu::notation::ViewMode::LINE);
    auto config = notation->interaction()->scoreConfig();
    config.isShowInvisibleElements = false;
    config.isShowUnprintableElements = false;
    config.isShowFrames = false;
    config.isShowPageMargins = false;
    config.isShowSoundFlags = false;
    notation->interaction()->setScoreConfig(config);
    constrainScorePosition();
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

void OrchestrionNotationPaintView::constrainScorePosition()
{
  // moveCanvasToPosition() below feeds back into this function via
  // onMatrixChanged(). Usually that re-entrant call lands on the same position
  // and stops, but at very low zoom our constraint and the base class's canvas
  // constraint disagree and never reach a common fixed point, so the recursion
  // overflows the stack. Guard against re-entry — a single pass is enough.
  if (m_constrainingScorePosition)
    return;
  m_constrainingScorePosition = true;

  const auto content = notationContentRect(); // logical
  const auto scaling = currentScaling();
  const auto emptyAbovePhysical = (height() - content.height() * scaling) / 2.;
  const auto topLogicalY = content.top() - emptyAbovePhysical / scaling;

  // two horizontal rules:
  // 1. not more than 100 empty pixels left or right
  // 2. if the score is narrower than the view, it stays centered
  constexpr double maxEmptyPhysical = 100.;
  const auto contentWidthPhysical = content.width() * scaling;
  double leftLogicalX;
  if (contentWidthPhysical < width())
    leftLogicalX =
        content.left() - (width() - contentWidthPhysical) / (2 * scaling);
  else
  {
    const auto minLeft = content.left() - maxEmptyPhysical / scaling;
    const auto maxLeft =
        content.right() + maxEmptyPhysical / scaling - width() / scaling;
    leftLogicalX = std::clamp(viewport().left(), minLeft, maxLeft);
  }

  moveCanvasToPosition(muse::PointF{leftLogicalX, topLogicalY});

  m_constrainingScorePosition = false;
}

void OrchestrionNotationPaintView::paintNotationUnderlay(QPainter *painter)
{
  // Called by the base view once the painter carries the score's world
  // transform and before the notation is drawn — so the highlight sits behind
  // the notes (but on top of the background), and we draw in logical
  // coordinates.
  if (m_boxes.empty())
    return;

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setPen(Qt::NoPen);
  painter->setOpacity(1.0);

  std::for_each(m_boxes.begin(), m_boxes.end(),
                [painter](const auto &entry)
                {
                  const Box &box = entry.second;
                  const QRectF &rect = box.rect;

                  // Soft highlighter-style fill: a translucent rounded block
                  // behind the notes, stronger for a ringing note, faint for
                  // the upcoming one.
                  QColor fill = box.color;
                  fill.setAlphaF(box.intensity * 0.3);
                  painter->setBrush(fill);

                  qreal r = box.spatium * 1.5;
                  r = std::min(r, rect.width() / 2);
                  r = std::min(r, rect.height() / 2);
                  painter->drawRoundedRect(rect, r, r);
                });
  painter->restore();
}

void OrchestrionNotationPaintView::paint(QPainter *painter)
{
  NotationPaintView::paint(painter);

  painter->restore();
  // Touch contacts stay on top of everything.
  painter->setRenderHint(QPainter::Antialiasing);
  painter->setBrush(Qt::NoBrush);
  painter->setOpacity(1.0);

  const auto view = viewport();

  const auto radius = 30. / currentScaling();

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