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
#include "IOrchestrionNotationInteractionProcessor.h"
#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionSequencer/IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSequencer/OrchestrionTypes.h"
#include "ScoreAnimation/ISegmentRegistry.h"
#include <actions/iactionsdispatcher.h>
#include <context/iglobalcontext.h>
#include <notation/inotationconfiguration.h>
#include <notation/view/notationpaintview.h>
#include <unordered_map>

namespace dgk
{
class OrchestrionNotationPaintView : public mu::notation::NotationPaintView
{
  Q_OBJECT
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

  //! Developer note-labeling feature: called back from the QML label editor
  //! shown after a right-click on a note (see onRightClicked).
  Q_INVOKABLE void commitNoteLabel(const QString &text);
  Q_INVOKABLE void cancelNoteLabel();

signals:
  void mouseActivity();
  //! Emitted on right-clicking a note while note-labeling mode is enabled.
  //! \a rect is the note's bounding rect in view coordinates.
  void noteLabelRequested(const QRectF &rect, const QString &currentText);

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
  void onMousePressed(const QPointF &pos);
  void onMouseMoved(const QPointF &pos);
  void onRightClicked(const QPointF &pos);
  std::vector<mu::engraving::EngravingItem *>
  getRelevantItems(TrackIndex track,
                   const mu::engraving::Segment *segment) const;
  void OnTransitions(const std::map<TrackIndex, ChordTransition> &transitions);
  void updateNotation();
  void wheelEvent(QWheelEvent *) override {}
  void initTouchpadMidiController();
  float hitWidth() const;

  struct Box
  {
    QRectF rect;
    QPen pen;
    double opacity = 1.0;
    bool active = false;
  };
  std::unordered_map<int, Box> m_boxes;

  struct Contact
  {
    const bool isLeft;
    double x = 0.;
    double y = 0.;
  };

  std::unordered_map<int, Contact> m_contacts;
  bool m_constrainingScorePosition = false;
  QPoint m_lastCursorPos{-1, -1};

  // Note under the cursor when the label editor was opened (note-labeling
  // feature). Valid only between onRightClicked and commit/cancel.
  const mu::notation::EngravingItem *m_labelTarget = nullptr;
};
} // namespace dgk