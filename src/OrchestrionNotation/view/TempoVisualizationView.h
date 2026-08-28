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

#include "TempoVizModel.h"

#include <QQuickPaintedItem>

namespace dgk
{
//! Real-time strip (shown beneath the score, toggled from the Advanced menu)
//! that plots the tempo model: per hand, estimated tempo vs recent real time,
//! with a tick at each onset the model reacted to, and coasting (overdue)
//! stretches drawn dashed. Debug-grade for now; expected to iterate.
class TempoVisualizationView : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(
      dgk::TempoVizModel *model READ model WRITE setModel NOTIFY modelChanged)

public:
  explicit TempoVisualizationView(QQuickItem *parent = nullptr);

  TempoVizModel *model() const { return _model; }
  void setModel(TempoVizModel *model);

  void paint(QPainter *painter) override;

signals:
  void modelChanged();

private:
  TempoVizModel *_model = nullptr;
};
} // namespace dgk
