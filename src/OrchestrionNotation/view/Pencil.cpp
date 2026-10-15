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
#include "Pencil.h"

#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
/** How dark the graphite is. */
constexpr double graphiteAlpha = 0.82;

/** A few pseudo-random numbers in [0, 1) from a seed, the same every time. */
class Wobble
{
public:
  explicit Wobble(unsigned seed) : m_state{seed * 2654435761u + 1u} {}
  double Next()
  {
    m_state = m_state * 1664525u + 1013904223u;
    return (m_state >> 8) / static_cast<double>(1u << 24);
  }

private:
  unsigned m_state;
};
} // namespace

void Pencil(QPainter &painter, const QPainterPath &path, const QColor &ink,
            double width, unsigned seed)
{
  // A fat stroke and a finer one over it, each a little off the line and off
  // each other: the uneven edge and density of graphite.
  constexpr double passWidth[] = {1.0, 0.55};
  constexpr double passAlpha[] = {0.7, 0.55};
  for (int pass = 0; pass < 2; ++pass)
  {
    Wobble wobble{seed * 7u + static_cast<unsigned>(pass)};
    const double amplitude = width * (pass == 0 ? 0.15 : 0.22);
    const double phase1 = wobble.Next() * 2 * M_PI;
    const double phase2 = wobble.Next() * 2 * M_PI;
    // Long next to a sketch: the line bends, it doesn't shake.
    const double wavelength = width * (10.0 + 4.0 * wobble.Next());
    const QPointF shift{(wobble.Next() - 0.5) * width * 0.3,
                        (wobble.Next() - 0.5) * width * 0.3};

    QColor colour = ink;
    colour.setAlphaF(graphiteAlpha * passAlpha[pass]);
    QPen pen{colour, width * passWidth[pass], Qt::SolidLine, Qt::RoundCap,
             Qt::RoundJoin};
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    for (const QPolygonF &polygon : path.toSubpathPolygons())
    {
      if (polygon.size() < 2)
        continue;
      // Resampled finely enough for the wander to be a curve.
      QPolygonF line;
      double along = 0;
      for (int i = 1; i < polygon.size(); ++i)
      {
        const QPointF a = polygon[i - 1];
        const QPointF b = polygon[i];
        const QPointF d = b - a;
        const double length = std::hypot(d.x(), d.y());
        if (length <= 0)
          continue;
        const QPointF normal{-d.y() / length, d.x() / length};
        const int steps = std::max(1, static_cast<int>(length / (width * 0.5)));
        for (int k = i == 1 ? 0 : 1; k <= steps; ++k)
        {
          const double t = static_cast<double>(k) / steps;
          const double s = (along + t * length) / wavelength * 2 * M_PI;
          const double off = amplitude * (std::sin(s + phase1) +
                                          0.5 * std::sin(2.3 * s + phase2));
          line << a + d * t + normal * off + shift;
        }
        along += length;
      }
      painter.drawPolyline(line);
    }
  }
}
} // namespace dgk
