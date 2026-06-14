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

#include <QColor>
#include <QElapsedTimer>
#include <QRectF>
#include <QTimer>
#include <functional>
#include <vector>

namespace dgk
{
//! The visual of a single note/chord highlight, in score-logical coordinates.
struct Highlight
{
  QRectF rect;
  QColor color;
  double spatium = 1.0;   // score-derived rounding/scale (logical units)
  double intensity = 1.0; // 0..1 base opacity (bright = ringing)
};

//! Owns the highlights of just-ended notes and fades them out over a fixed
//! duration. The owner hands a finished highlight to add(); the fader keeps a
//! ~60 fps timer running while any highlight is fading, calls the supplied
//! repaint callback on each tick, and drops highlights once fully faded
//! (stopping the timer when none remain, so it costs nothing while idle).
//!
//! It does not paint: paint() the owner calls forEach() to draw each fading
//! highlight at its current opacity, reusing whatever fill routine it uses for
//! live highlights.
class HighlightFader
{
public:
  //! \p requestRepaint is invoked (on the owning thread) whenever the fading
  //! highlights change and the view should repaint. \p fadeMs is the fade-out
  //! duration.
  explicit HighlightFader(std::function<void()> requestRepaint,
                          qint64 fadeMs = 150);

  HighlightFader(const HighlightFader &) = delete;
  HighlightFader &operator=(const HighlightFader &) = delete;

  //! Start fading \p highlight out from now.
  void add(Highlight highlight);

  //! Drop all fading highlights and stop animating (e.g. on a score jump or
  //! notation reload). Does not request a repaint.
  void clear();

  bool empty() const { return m_entries.empty(); }

  //! Call \p draw for each fading highlight with its current opacity factor
  //! (1 at note-off, ramping to 0 at the end of the fade).
  void forEach(const std::function<void(const Highlight &, double opacity)>
                   &draw) const;

private:
  //! Timer tick: drop fully-faded highlights, stop when none remain, repaint.
  void advance();

  const std::function<void()> m_requestRepaint;
  const qint64 m_fadeMs;
  QElapsedTimer m_clock; // free-running; timestamps when each fade started
  QTimer m_timer;        // ~60 Hz tick while fading

  struct Entry
  {
    Highlight highlight;
    qint64 startMs; // m_clock timestamp at which the fade began
  };
  std::vector<Entry> m_entries;
};
} // namespace dgk
