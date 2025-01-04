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
#include <QApplication>

namespace dgk
{
OrchestrionNotationPaintView::OrchestrionNotationPaintView(QQuickItem *parent)
    : mu::notation::NotationPaintView(parent)
{
  qApp->installEventFilter(this);
}

bool OrchestrionNotationPaintView::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == this && event->type() == QEvent::MouseButtonPress)
  {
    const auto mouseEvent = static_cast<QMouseEvent *>(event);
    onMousePressed(mouseEvent->localPos());
  }
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
} // namespace dgk