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
#pragma once

#include "GestureControllers/IGestureControllerSelector.h"
#include "HighlightFader.h"
#include "IOrchestrionNotationInteractionProcessor.h"
#include "KineticScroller.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"
#include "ScoreAnimation/ISegmentRegistry.h"
#include "TempoFollower.h"
#include <actions/iactionsdispatcher.h>
#include <context/iglobalcontext.h>
#include <notation/inotationconfiguration.h>
#include <notation/view/notationpaintview.h>
#include <unordered_map>
#include <vector>

namespace dgk
{
class OrchestrionNotationPaintView : public mu::notation::NotationPaintView,
                                     public TempoFollower::Canvas
{
  Q_OBJECT
  // Debug tooltip shown when hovering a note (see noteInfoTooltipEnabled).
  // Empty when nothing relevant is hovered, which hides the tooltip.
  Q_PROPERTY(QString hoveredNoteInfo READ hoveredNoteInfo NOTIFY
                 hoveredNoteInfoChanged)
  Q_PROPERTY(QPointF hoveredNoteInfoPos READ hoveredNoteInfoPos NOTIFY
                 hoveredNoteInfoChanged)

  muse::Inject<IOrchestrionNotationInteractionProcessor> interactionProcessor;
  muse::Inject<mu::notation::INotationConfiguration> configuration;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<IGestureControllerSelector> gestureControllerSelector;
  muse::Inject<ISegmentRegistry> chordRegistry;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;
  muse::Inject<IOrchestrionSequencerConfiguration> sequencerConfiguration;

public:
  explicit OrchestrionNotationPaintView(QQuickItem *parent = nullptr);

  Q_INVOKABLE void loadOrchestrionNotation();

  QString hoveredNoteInfo() const;
  QPointF hoveredNoteInfoPos() const;

signals:
  void mouseActivity();
  void hoveredNoteInfoChanged();

private:
  void onLoadNotation(mu::notation::INotationPtr notation) override;
  void onMatrixChanged(const muse::draw::Transform &oldMatrix,
                       const muse::draw::Transform &newMatrix,
                       bool overrideZoomType = true) override;
  void subscribe(const IOrchestrionSequencer &sequencer,
                 const IModifiableItemRegistry &registry);
  void constrainScorePosition();
  void setViewMode(mu::notation::ViewMode);
  bool eventFilter(QObject *watched, QEvent *event) override;
  void paint(QPainter *painter) override;
  void paintNotationUnderlay(QPainter *painter) override;
  void onMousePressed(const QPointF &pos, Qt::KeyboardModifiers modifiers,
                      Qt::MouseButton button);
  void onMouseDragged(const QPointF &pos, Qt::MouseButtons buttons);
  void onMouseReleased(Qt::MouseButton button);
  void onMouseMoved(const QPointF &pos);
  void updateHoveredNoteInfo(const QPointF &itemPos);
  void setHoveredNoteInfo(const QString &info, const QPointF &itemPos);
  std::vector<mu::engraving::EngravingItem *>
  getRelevantItems(TrackIndex track,
                   const mu::engraving::Segment *segment) const;
  void OnTransitions(const std::map<TrackIndex, ChordTransition> &transitions);
  void updateNotation();

  // TempoFollower::Canvas
  double viewWidth() const override { return width(); }
  double viewScaling() const override { return currentScaling(); }
  double minScaling() const override;
  void centerOn(double logicalX, double scaling) override;
  void wheelEvent(QWheelEvent *event) override;
  //! Zoom the score in/out about the cursor in response to a Ctrl-modified
  //! wheel event (mouse wheel or two-finger trackpad swipe).
  void zoomBy(const QWheelEvent &event);
  //! Pan the canvas by \p physicalDx physical pixels; returns whether it moved
  //! (false ⇒ clamped at an edge). Drives the KineticScroller.
  bool moveCanvasBy(qreal physicalDx);
  void initTouchpadMidiController();
  float hitWidth() const;

  // Live highlight per track (ringing or upcoming note).
  std::unordered_map<int, Highlight> m_boxes;
  // Highlights of just-ended notes, fading out (owns its own timer/clock).
  HighlightFader m_fader;

  struct Contact
  {
    const bool isLeft;
    double x = 0.;
    double y = 0.;
  };

  std::unordered_map<int, Contact> m_contacts;
  bool m_constrainingScorePosition = false;
  QPoint m_lastCursorPos{-1, -1};

  // Constant-speed score following (single-hand, first pass): turns played
  // onsets into a smooth scroll. While it drives the canvas (centerOn),
  // m_drivingScroll makes constrainScorePosition() yield so it isn't undone.
  TempoFollower m_follower;
  bool m_drivingScroll = false;

  // Background left-drag pans the canvas (done by the base view); we sample the
  // drag so releasing it adds a kinetic throw via m_kineticScroller.
  bool m_canvasDragging = false;
  QPointF m_lastDragPos;

  // Kinetic ("flick") horizontal scrolling: a trackpad swipe can be "thrown"
  // and the viewport keeps gliding until it slows to a stop or hits the edge.
  KineticScroller m_kineticScroller;
  QString m_hoveredNoteInfo;
  QPointF m_hoveredNoteInfoPos;
};
} // namespace dgk