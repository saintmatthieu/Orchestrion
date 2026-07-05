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

#include "TempoFollower.h"

#include <QObject>

#include <deque>
#include <map>

namespace dgk
{
//! Rolling buffer of what the tempo model is doing, fed by TempoFollower (via
//! its VizSink) and read by TempoVisualizationView. Per hand (staff) it keeps a
//! recent history of tempo samples and the onset times the model reacted to,
//! within a fixed time window. Emits changed() so the view repaints. Owned by
//! the paint view and handed to the view as a (non-owning) QML property.
class TempoVizModel : public QObject, public TempoFollower::VizSink
{
  Q_OBJECT

public:
  explicit TempoVizModel(QObject *parent = nullptr);

  //! Length of the visible/retained history.
  static constexpr double windowMs = 8000.0;

  struct Point
  {
    double tMs;
    double tempo;
    bool coasting;
  };

  // Read API for the view (newest entries at the back). series() is the
  // causal per-frame estimate; smoothed() the re-fitted spline per hand,
  // replaced wholesale on each of that hand's onsets.
  const std::map<int, std::deque<Point>> &series() const { return _series; }
  const std::map<int, std::vector<CurvePoint>> &smoothed() const
  {
    return _smoothed;
  }
  const std::map<int, std::deque<double>> &onsets() const { return _onsets; }
  double latestMs() const { return _latestMs; }
  bool empty() const { return _series.empty() && _onsets.empty(); }

  //! Drop all history (e.g. a position jump or new score).
  void clear();

  // TempoFollower::VizSink
  void onTempoSample(double tMs, const std::vector<HandTempo> &hands) override;
  void onOnset(double tMs, int staff) override;
  void onSmoothedTempo(int staff, const std::vector<CurvePoint> &curve) override;

signals:
  void changed();

private:
  void prune();

  std::map<int /*staff*/, std::deque<Point>> _series;
  std::map<int /*staff*/, std::vector<CurvePoint>> _smoothed;
  std::map<int /*staff*/, std::deque<double>> _onsets;
  double _latestMs = 0.0;
};
} // namespace dgk
