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
#include "Transcription.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <log.h>

namespace dgk
{
namespace
{
constexpr int kTranscriptionVersion = 1;
}

QString TranscriptionPath(const QString &userDataPath, const QString &pieceId)
{
  return userDataPath + "/performances/" + pieceId + ".json";
}

std::optional<std::vector<PerformedNote>> ReadTranscription(const QString &path)
{
  QFile file{path};
  if (!file.open(QIODevice::ReadOnly))
    return std::nullopt;
  const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
  if (root.value("version").toInt() != kTranscriptionVersion)
  {
    LOGW() << "Not a transcription this version reads: " << path;
    return std::nullopt;
  }
  std::vector<PerformedNote> notes;
  for (const QJsonValue &value : root.value("notes").toArray())
  {
    const QJsonObject note = value.toObject();
    const double seconds = note.value("time").toDouble(-1.0);
    const double duration = note.value("duration").toDouble(-1.0);
    const int pitch = note.value("midi").toInt(-1);
    const int velocity = note.value("velocity").toInt(-1);
    if (seconds < 0.0 || duration < 0.0 || pitch < 0 || pitch > 127 ||
        velocity < 0 || velocity > 127)
    {
      LOGW() << "A note of a transcription isn't one: " << path;
      return std::nullopt;
    }
    notes.push_back(
        {seconds * 1000.0, (seconds + duration) * 1000.0, pitch, velocity});
  }
  return notes;
}
} // namespace dgk
