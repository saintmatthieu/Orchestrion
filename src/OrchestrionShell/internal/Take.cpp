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
#include "Take.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace dgk
{
namespace
{
const char *KindName(Take::Event::Kind kind)
{
  switch (kind)
  {
  case Take::Event::Kind::noteOn:
    return "noteOn";
  case Take::Event::Kind::noteOff:
    return "noteOff";
  case Take::Event::Kind::sustainPedal:
    return "sustainPedal";
  case Take::Event::Kind::playbackStarted:
    return "playbackStarted";
  case Take::Event::Kind::playbackStopped:
    return "playbackStopped";
  }
  return "noteOn";
}

std::optional<Take::Event::Kind> KindOf(const QString &name)
{
  for (const auto kind :
       {Take::Event::Kind::noteOn, Take::Event::Kind::noteOff,
        Take::Event::Kind::sustainPedal, Take::Event::Kind::playbackStarted,
        Take::Event::Kind::playbackStopped})
    if (name == KindName(kind))
      return kind;
  return std::nullopt;
}

const char *ModeName(StaffMode mode)
{
  switch (mode)
  {
  case StaffMode::notes:
    return "notes";
  case StaffMode::rhythm:
    return "rhythm";
  case StaffMode::follow:
    return "follow";
  case StaffMode::mute:
    return "mute";
  }
  return "rhythm";
}

StaffMode ModeOf(const QString &name)
{
  for (const auto mode : {StaffMode::notes, StaffMode::rhythm,
                          StaffMode::follow, StaffMode::mute})
    if (name == ModeName(mode))
      return mode;
  return StaffMode::rhythm;
}
} // namespace

int Take::NoteCount() const
{
  return static_cast<int>(
      std::count_if(events.begin(), events.end(), [](const Event &e)
                    { return e.kind == Event::Kind::noteOn; }));
}

double Take::DurationMs() const
{
  return events.empty() ? 0.0 : events.back().ms;
}

QByteArray ToJson(const Take &take)
{
  QJsonArray events;
  for (const Take::Event &e : take.events)
  {
    QJsonObject o{{"ms", e.ms}, {"kind", KindName(e.kind)}};
    switch (e.kind)
    {
    case Take::Event::Kind::noteOn:
      if (e.velocity)
        o["velocity"] = *e.velocity;
      [[fallthrough]];
    case Take::Event::Kind::noteOff:
      o["pitch"] = e.pitch;
      break;
    case Take::Event::Kind::sustainPedal:
      o["down"] = e.pedalDown;
      break;
    default:
      break;
    }
    events.append(o);
  }
  const QJsonObject root{
      {"version", 1},
      {"score", take.scorePath},
      {"startTick", take.startTick},
      {"startBar", take.startBar},
      {"staffModes",
       QJsonArray{ModeName(take.staffModes[0]), ModeName(take.staffModes[1])}},
      {"ornamentMode", static_cast<int>(take.ornamentMode)},
      {"loop", QJsonObject{{"enabled", take.loopEnabled},
                           {"inTick", take.loopInTick},
                           {"outTick", take.loopOutTick}}},
      {"events", events}};
  return QJsonDocument{root}.toJson();
}

std::optional<Take> TakeFromJson(const QByteArray &json)
{
  const QJsonDocument document = QJsonDocument::fromJson(json);
  if (!document.isObject())
    return std::nullopt;
  const QJsonObject root = document.object();
  if (root["version"].toInt() != 1)
    return std::nullopt;
  Take take;
  take.scorePath = root["score"].toString();
  take.startTick = root["startTick"].toInt();
  take.startBar = root["startBar"].toInt(1);
  const QJsonArray modes = root["staffModes"].toArray();
  for (int i = 0; i < 2 && i < modes.size(); ++i)
    take.staffModes[i] = ModeOf(modes[i].toString());
  take.ornamentMode = static_cast<OrnamentMode>(
      root["ornamentMode"].toInt(static_cast<int>(OrnamentMode::automatic)));
  const QJsonObject loop = root["loop"].toObject();
  take.loopEnabled = loop["enabled"].toBool();
  take.loopInTick = loop["inTick"].toInt();
  take.loopOutTick = loop["outTick"].toInt();
  for (const QJsonValue &value : root["events"].toArray())
  {
    const QJsonObject o = value.toObject();
    const auto kind = KindOf(o["kind"].toString());
    if (!kind)
      continue;
    Take::Event e;
    e.ms = o["ms"].toDouble();
    e.kind = *kind;
    e.pitch = o["pitch"].toInt();
    if (o.contains("velocity"))
      e.velocity = static_cast<float>(o["velocity"].toDouble());
    e.pedalDown = o["down"].toBool();
    take.events.push_back(e);
  }
  return take;
}
} // namespace dgk
