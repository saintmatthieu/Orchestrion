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
#pragma once

#include "OrchestrionSequencer/IStaffModes.h"

#include <QColor>
#include <QPointF>
#include <QRectF>
#include <optional>
#include <utility>
#include <vector>

class QPainter;
class QPainterPath;

namespace dgk
{
/**
 * The choice of the staff modes (see IStaffModes), pencilled on the
 * parchment before each staff: the staff's mode as a little sketch — a note
 * for Notes, a percussion note for Rhythm, a matchstick man for Follow, a
 * cross for Mute.
 * Two arrows to its left step through the modes, up to the previous one and
 * down to the next, round and round.
 *
 * Everything is in the coordinates it is painted in — the score's.
 */
class StaffModeSelector
{
public:
  struct Staff
  {
    int staff = 0;
    /** The middle of the sketch: before the staff, level with its middle. */
    QPointF centre;
    StaffMode mode = StaffMode::rhythm;
  };

  struct Layout
  {
    std::vector<Staff> staves;
    std::vector<StaffMode> available;
    /** How tall a sketch is; the arrows are sized and placed after it. */
    double size = 0;
  };

  /** What of a staff's choice there is at a point. */
  enum class Part
  {
    sketch,
    previous,
    next,
  };

  struct Hit
  {
    int staff = 0;
    StaffMode mode = StaffMode::rhythm;
    Part part = Part::sketch;
  };

  void paint(QPainter &, const Layout &, const QColor &ink) const;

  /**
   * The pointer is at `pos`, or has left: returns whether what is shown
   * changed.
   */
  bool hover(const Layout &, std::optional<QPointF> pos);

  /** What is at `pos`, if anything. */
  std::optional<Hit> hitAt(const Layout &, QPointF pos) const;

  /**
   * The mode an arrow steps to from `mode`, round the available ones; `mode`
   * itself for the sketch.
   */
  static StaffMode Step(const std::vector<StaffMode> &available, StaffMode mode,
                        Part);

private:
  static QPointF ArrowCentre(const Layout &, const Staff &, Part);
  static bool IsOver(const Layout &, const Staff &, Part, QPointF pos);
  static void Sketch(QPainter &, StaffMode, QPointF centre, double size,
                     const QColor &ink);
  static void Arrow(QPainter &, Part, QPointF centre, double size,
                    const QColor &ink);

  /** The staff whose choice is pointed at, and what of it. */
  std::optional<int> m_pointedStaff;
  std::optional<Part> m_pointedPart;
};
} // namespace dgk
