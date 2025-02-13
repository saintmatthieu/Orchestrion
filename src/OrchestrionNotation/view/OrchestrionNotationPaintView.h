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

#include "IOrchestrionNotationInteraction.h"
#include "Orchestrion/IOrchestrion.h"
#include "Orchestrion/OrchestrionTypes.h"
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
  muse::Inject<IOrchestrionNotationInteraction> orchestrionNotationInteraction;
  muse::Inject<mu::notation::INotationConfiguration> configuration;
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<IOrchestrion> orchestrion;
  muse::Inject<ISegmentRegistry> chordRegistry;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;

public:
  explicit OrchestrionNotationPaintView(QQuickItem *parent = nullptr);

  Q_INVOKABLE void loadOrchestrionNotation();

private:
  void subscribe(const IOrchestrionSequencer &sequencer);
  void alignVertically();
  void setViewMode(mu::notation::ViewMode);
  bool eventFilter(QObject *watched, QEvent *event) override;
  void paint(QPainter *painter) override;
  void onMousePressed(const QPointF &pos);
  std::vector<mu::engraving::EngravingItem *>
  getRelevantItems(TrackIndex track,
                   const mu::engraving::Segment *segment) const;
  void OnTransitions(const std::map<TrackIndex, ChordTransition> &transitions);
  void updateNotation();

  struct Box
  {
    QRectF rect;
    QPen pen;
    double opacity = 1.0;
    bool active = false;
  };
  std::unordered_map<int, Box> m_boxes;
};
} // namespace dgk