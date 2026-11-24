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
#include "RecordingSync.h"

#include <engraving/dom/measure.h>
#include <engraving/dom/repeatlist.h>
#include <engraving/dom/score.h>
#include <engraving/dom/segment.h>
#include <log.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <algorithm>
#include <cmath>
#include <map>

namespace dgk
{
namespace
{
constexpr int kFormatVersion = 2;
/** A whole note, in MuseScore's ticks. */
constexpr int kWholeTicks = 4 * 480;
/**
 * Passes of SmoothBeats over taps by hand: their noise smoothed out, for
 * following and for playing as the recording does.
 */
constexpr int kSmoothing = 2;

/**
 * The time signature's beat: a dotted quarter in 6/8, 9/8 and 12/8 (three
 * eighths), else its denominator's note (a half in cut time).
 */
int BeatTicks(const mu::engraving::Fraction &timesig)
{
  const int note = kWholeTicks / std::max(1, timesig.denominator());
  const bool compound = timesig.numerator() > 3 &&
                        timesig.numerator() % 3 == 0 &&
                        timesig.denominator() >= 8;
  return compound ? 3 * note : note;
}

} // namespace

std::vector<PlayedBar> PlayedBars(const mu::engraving::Score &score)
{
  // As the sequencer walks them (ForAllMeasures).
  std::vector<PlayedBar> bars;
  std::map<const mu::engraving::Measure *, int> passes;
  int tick = 0;
  for (const auto *repeatSegment : score.repeatList(true))
    for (const mu::engraving::Measure *measure : repeatSegment->measureList())
    {
      PlayedBar bar;
      bar.tick = tick;
      bar.writtenTick = measure->tick().ticks();
      bar.length = measure->ticks().ticks();
      bar.beatTicks = BeatTicks(measure->timesig());
      bar.beats = (bar.length + bar.beatTicks - 1) / bar.beatTicks;
      bar.number = measure->measureNumber() + 1; // as the layout prints it
      bar.pickup = bars.empty() && measure->ticks() < measure->timesig();
      if (bar.pickup)
        bar.gridOffset = (measure->timesig() - measure->ticks()).ticks();
      bar.pass = ++passes[measure];
      for (const mu::engraving::Segment *segment =
               measure->first(mu::engraving::SegmentType::ChordRest);
           segment;
           segment = segment->next(mu::engraving::SegmentType::ChordRest))
        if (std::any_of(segment->elist().begin(), segment->elist().end(),
                        [](const mu::engraving::EngravingItem *item)
                        { return item && item->isChord(); }))
          bar.lastOnset = (segment->tick() - measure->tick()).ticks();
      bars.push_back(bar);
      tick += bar.length;
    }
  return bars;
}

QString RecordingSyncPath(const QString &userDataPath, const QString &pieceId,
                          const QString &videoId)
{
  return userDataPath + "/sync/" + pieceId + "/" + videoId + ".json";
}

std::optional<RecordingSync>
ReadRecordingSync(const QString &path, const std::vector<PlayedBar> &bars)
{
  QFile file{path};
  if (!file.open(QIODevice::ReadOnly))
    return std::nullopt;
  const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
  const int version = root.value("version").toInt();
  if (version != 1 && version != kFormatVersion)
  {
    LOGW() << "Not a sync file this version reads: " << path;
    return std::nullopt;
  }
  RecordingSync sync;
  sync.scoreVersion = root.value("score").toString();
  for (const QJsonValue &value : root.value("points").toArray())
  {
    const QJsonArray point = value.toArray();
    const int bar = point[0].toInt();
    if (point.size() != 3 || bar < 0 || bar >= static_cast<int>(bars.size()))
    {
      LOGW() << "A sync point isn't [bar, tick, seconds] in the score: "
             << path;
      return std::nullopt;
    }
    const int tick = version == 1 ? (point[1].toInt() - 1) * bars[bar].beatTicks
                                  : point[1].toInt();
    sync.points.push_back({bar, tick, point[2].toDouble()});
  }
  return sync;
}

bool WriteRecordingSync(const QString &path, const RecordingSync &sync)
{
  QJsonArray points;
  for (const RecordingSync::Point &point : sync.points)
    points.append(QJsonArray{point.bar, point.tick,
                             std::round(point.seconds * 1000.0) / 1000.0});
  QJsonObject root;
  root["version"] = kFormatVersion;
  root["score"] = sync.scoreVersion;
  root["points"] = points;

  if (!QDir{}.mkpath(QFileInfo{path}.absolutePath()))
    return false;
  QSaveFile file{path};
  if (!file.open(QIODevice::WriteOnly))
    return false;
  file.write(QJsonDocument{root}.toJson(QJsonDocument::Compact));
  return file.commit();
}

std::shared_ptr<const RecordingTimeline>
MakeRecordingTimeline(const mu::engraving::Score &score,
                      const RecordingSync &sync)
{
  const std::vector<PlayedBar> bars = PlayedBars(score);
  std::vector<RecordingBeat> beats;
  std::optional<int> previousBar;
  for (const RecordingSync::Point &point : sync.points)
  {
    if (point.bar < 0 || point.bar >= static_cast<int>(bars.size()) ||
        point.tick < 0)
      return nullptr;
    const PlayedBar &bar = bars[point.bar];
    const int offset = point.tick;
    if (offset >= bar.length)
      return nullptr;
    beats.push_back({bar.tick + offset, bar.writtenTick + offset, bar.beatTicks,
                     point.seconds,
                     previousBar && point.bar > *previousBar + 1});
    previousBar = point.bar;
  }
  std::vector<RecordingBar> played;
  played.reserve(bars.size());
  for (const PlayedBar &bar : bars)
    played.push_back({bar.tick, bar.writtenTick, bar.length});
  auto timeline = std::make_shared<const RecordingTimeline>(
      SmoothBeats(std::move(beats), kSmoothing), std::move(played));
  return timeline->Empty() ? nullptr : timeline;
}
} // namespace dgk
