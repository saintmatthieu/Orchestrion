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

namespace dgk::orchestrion
{
OrchestrionNotationPaintView::OrchestrionNotationPaintView(QQuickItem *parent)
    : mu::notation::NotationPaintView(parent)
{
}

void OrchestrionNotationPaintView::init()
{
  m_controller.pageSizeChanged().onNotify(this,
                                          [this]() { emit pageSizeChanged(); });
}

void OrchestrionNotationPaintView::loadOrchestrionNotation()
{
  load();
  // So that we have one long horizontal scrolling view.
  dispatcher()->dispatch("view-mode-continuous");
  alignVertically();
}

QSize OrchestrionNotationPaintView::pageSize() const
{
  const auto size = m_controller.pageSize();
  return QSize(size.width(), size.height());
}

void OrchestrionNotationPaintView::alignVertically()
{
  const auto canvasRect = fromLogical(notationContentRect());
  const auto y = (height() - canvasRect.height()) / 2.;
  // Don't know why we need to negate.
  moveCanvasToPosition(toLogical(mu::PointF{-y, -y}));
}
} // namespace dgk::orchestrion