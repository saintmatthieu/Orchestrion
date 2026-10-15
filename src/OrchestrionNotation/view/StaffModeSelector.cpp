/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
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
#include "StaffModeSelector.h"
#include "Pencil.h"

#include <QPainter>
#include <QPainterPath>
#include <QTransform>
#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
/** How faint an arrow not pointed at is. */
constexpr double idleArrowOpacity = 0.5;
/** The pencil's width, as a fraction of a sketch's height. */
constexpr double pencilWidthRatio = 0.12;
/**
 * The arrows, as fractions of a sketch's height: how far left of it they
 * are, how far above and below its middle, and how big — the area that
 * takes the click as much as the arrow drawn.
 */
constexpr double arrowOffsetX = 0.95;
constexpr double arrowOffsetY = 0.28;
constexpr double arrowHalfWidth = 0.3;
constexpr double arrowHalfHeight = 0.27;
/** The sketch's area, as a fraction of its height, either way from its middle.
 */
constexpr double sketchHalfExtent = 0.6;

} // namespace

QPointF StaffModeSelector::ArrowCentre(const Layout &layout, const Staff &staff,
                                       Part part)
{
  if (part == Part::sketch)
    return staff.centre;
  const double dy = arrowOffsetY * layout.size;
  return staff.centre + QPointF{-arrowOffsetX * layout.size,
                                part == Part::previous ? -dy : dy};
}

bool StaffModeSelector::IsOver(const Layout &layout, const Staff &staff,
                               Part part, QPointF pos)
{
  const QPointF centre = ArrowCentre(layout, staff, part);
  const double halfWidth =
      (part == Part::sketch ? sketchHalfExtent : arrowHalfWidth) * layout.size;
  const double halfHeight =
      (part == Part::sketch ? sketchHalfExtent : arrowHalfHeight) * layout.size;
  return std::abs(pos.x() - centre.x()) <= halfWidth &&
         std::abs(pos.y() - centre.y()) <= halfHeight;
}

StaffMode StaffModeSelector::Step(const std::vector<StaffMode> &available,
                                  StaffMode mode, Part part)
{
  const auto it = std::find(available.begin(), available.end(), mode);
  if (part == Part::sketch || available.empty() || it == available.end())
    return mode;
  const auto n = static_cast<long>(available.size());
  const auto index = static_cast<long>(it - available.begin());
  const auto step = part == Part::previous ? n - 1 : 1;
  return available[static_cast<size_t>((index + step) % n)];
}

void StaffModeSelector::paint(QPainter &painter, const Layout &layout,
                              const QColor &ink) const
{
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  for (const Staff &staff : layout.staves)
  {
    painter.setOpacity(1.0);
    Sketch(painter, staff.mode, staff.centre, layout.size, ink);
    if (layout.available.size() < 2)
      continue;
    for (const auto part : {Part::previous, Part::next})
    {
      painter.setOpacity(
          m_pointedStaff == staff.staff && m_pointedPart == part
              ? 1.0
              : idleArrowOpacity);
      Arrow(painter, part, ArrowCentre(layout, staff, part), layout.size, ink);
    }
  }
  painter.restore();
}

bool StaffModeSelector::hover(const Layout &layout, std::optional<QPointF> pos)
{
  const auto staff = m_pointedStaff;
  const auto part = m_pointedPart;
  const auto hit = pos ? hitAt(layout, *pos) : std::nullopt;
  m_pointedStaff = hit ? std::make_optional(hit->staff) : std::nullopt;
  m_pointedPart = hit ? std::make_optional(hit->part) : std::nullopt;
  return staff != m_pointedStaff || part != m_pointedPart;
}

std::optional<StaffModeSelector::Hit>
StaffModeSelector::hitAt(const Layout &layout, QPointF pos) const
{
  for (const Staff &staff : layout.staves)
  {
    if (IsOver(layout, staff, Part::sketch, pos))
      return Hit{staff.staff, staff.mode, Part::sketch};
    if (layout.available.size() > 1)
      for (const auto part : {Part::previous, Part::next})
        if (IsOver(layout, staff, part, pos))
          return Hit{staff.staff, staff.mode, part};
  }
  return std::nullopt;
}

void StaffModeSelector::Sketch(QPainter &painter, StaffMode mode, QPointF c,
                               double size, const QColor &ink)
{
  const double width = size * pencilWidthRatio;
  const auto at = [&](double x, double y)
  { return c + QPointF{x * size, y * size}; };
  QPainterPath path;
  unsigned seed = 0;
  switch (mode)
  {
  case StaffMode::notes:
  {
    // A note: a slanted oval head, scribbled in, its stem up from its right.
    const QPointF head = at(-0.1, 0.27);
    const double rx = 0.16 * size;
    const double ry = 0.11 * size;
    QTransform slant;
    slant.translate(head.x(), head.y());
    slant.rotate(-22);
    QPainterPath oval;
    oval.addEllipse(QPointF{}, rx, ry);
    // The fill, back and forth across the oval as a pencil fills it.
    for (const double y : {-0.55, -0.15, 0.25, 0.6})
    {
      const double x = rx * std::sqrt(1.0 - y * y);
      oval.moveTo(-x, y * ry);
      oval.lineTo(x, y * ry);
    }
    path.addPath(slant.map(oval));
    const QPointF right = slant.map(QPointF{rx * 0.92, -ry * 0.25});
    path.moveTo(right);
    path.lineTo(QPointF{right.x() + 0.01 * size, at(0, -0.46).y()});
    seed = 4;
    break;
  }
  case StaffMode::rhythm:
  {
    // A percussion note: a cross for a head, its stem up from its right.
    const QPointF head = at(-0.1, 0.27);
    const double h = 0.14 * size;
    path.moveTo(head + QPointF{-h, -h});
    path.lineTo(head + QPointF{h, h});
    path.moveTo(head + QPointF{-h, h});
    path.lineTo(head + QPointF{h, -h});
    path.moveTo(head + QPointF{h, -h});
    path.lineTo(at(0.04, -0.46));
    seed = 1;
    break;
  }
  case StaffMode::follow:
  {
    // A matchstick man.
    const double r = 0.12 * size;
    path.addEllipse(at(0, -0.33), r, r * 1.08);
    path.moveTo(at(0, -0.33) + QPointF{0, r * 1.08});
    path.lineTo(at(0.01, 0.12));
    path.moveTo(at(-0.22, 0.05));
    path.lineTo(at(0, -0.12));
    path.lineTo(at(0.22, 0.04));
    path.moveTo(at(-0.18, 0.45));
    path.lineTo(at(0.01, 0.12));
    path.lineTo(at(0.19, 0.44));
    seed = 2;
    break;
  }
  case StaffMode::mute:
    // A cross, the second stroke overshooting a touch as a quick hand does.
    path.moveTo(at(-0.3, -0.3));
    path.lineTo(at(0.29, 0.31));
    path.moveTo(at(0.31, -0.32));
    path.lineTo(at(-0.3, 0.33));
    seed = 3;
    break;
  }
  Pencil(painter, path, ink, width, seed);
}

void StaffModeSelector::Arrow(QPainter &painter, Part part, QPointF c,
                              double size, const QColor &ink)
{
  // A chevron, pointing up for the previous mode and down for the next.
  const double w = 0.25 * size;
  const double h = (part == Part::previous ? -0.13 : 0.13) * size;
  QPainterPath path;
  path.moveTo(c + QPointF{-w, -h});
  path.lineTo(c + QPointF{0, h});
  path.lineTo(c + QPointF{w, -h});
  Pencil(painter, path, ink, 0.9 * size * pencilWidthRatio,
         part == Part::previous ? 11u : 13u);
}
} // namespace dgk
