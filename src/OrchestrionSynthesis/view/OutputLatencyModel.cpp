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
#include "OutputLatencyModel.h"

#include <QVariantList>

namespace dgk
{
namespace
{
template <typename T> QVariantList toList(const std::vector<T> &values)
{
  QVariantList list;
  list.reserve(static_cast<qsizetype>(values.size()));
  for (const T &value : values)
    list.push_back(QVariant::fromValue(value));
  return list;
}
} // namespace

OutputLatencyModel::OutputLatencyModel(QObject *parent) : QObject{parent}
{
  if (outputLatency())
    outputLatency()->statusChanged().onNotify(this,
                                              [this] { emit statusChanged(); });
}

QVariantMap OutputLatencyModel::status() const
{
  if (!outputLatency())
    return {};
  const IOutputLatency::Status s = outputLatency()->status();
  return {
      {"available", s.available},
      {"driver", QString::fromStdString(s.driver)},
      {"periodMs", s.periodMs},
      {"levelsMarginMs", toList(s.levelsMarginMs)},
      {"automatic", s.automatic},
      {"level", static_cast<int>(s.level)},
      {"appliedLevel", static_cast<int>(s.appliedLevel)},
      {"headroomMeanMs", s.headroomMeanMs},
      {"headroomMinMs", s.headroomMinMs},
      {"loadMeanPercent", s.loadMeanPercent},
      {"loadMaxPercent", s.loadMaxPercent},
      {"underruns", static_cast<qulonglong>(s.underruns)},
      {"clockStalls", static_cast<qulonglong>(s.clockStalls)},
      {"secondsSinceLastUnderrun", s.secondsSinceLastUnderrun},
      {"underrunsPerSecond", toList(s.underrunsPerSecond)},
      {"clockStallsPerSecond", toList(s.clockStallsPerSecond)},
  };
}

void OutputLatencyModel::setAutomatic(bool automatic)
{
  if (outputLatency())
    outputLatency()->setAutomatic(automatic);
}

void OutputLatencyModel::setLevel(int level)
{
  if (outputLatency() && level >= 0)
    outputLatency()->setLevel(static_cast<size_t>(level));
}

void OutputLatencyModel::resetCounters()
{
  if (outputLatency())
    outputLatency()->resetCounters();
}

void OutputLatencyModel::delayNextCallback(double ms)
{
  if (outputLatency())
    outputLatency()->delayNextCallback(ms);
}
} // namespace dgk
