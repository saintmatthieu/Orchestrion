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
#include "TempoVizModel.h"

namespace dgk
{
TempoVizModel::TempoVizModel(QObject *parent) : QObject(parent) {}

void TempoVizModel::onTempoSample(double tMs, const std::vector<HandTempo> &hands)
{
  _latestMs = tMs;
  for (const auto &h : hands)
    _series[h.staff].push_back({tMs, h.tempo, h.coasting});
  prune();
  emit changed();
}

void TempoVizModel::onOnset(double tMs, int staff)
{
  _latestMs = std::max(_latestMs, tMs);
  _onsets[staff].push_back(tMs);
  prune();
  // No changed() here: a tempo sample (every frame) follows promptly and
  // repaints; avoids redundant repaints on the onset thread.
}

void TempoVizModel::prune()
{
  const double cutoff = _latestMs - windowMs;
  for (auto &[staff, pts] : _series)
    while (!pts.empty() && pts.front().tMs < cutoff)
      pts.pop_front();
  for (auto &[staff, ts] : _onsets)
    while (!ts.empty() && ts.front() < cutoff)
      ts.pop_front();
}

void TempoVizModel::clear()
{
  _series.clear();
  _onsets.clear();
  _latestMs = 0.0;
  emit changed();
}
} // namespace dgk
