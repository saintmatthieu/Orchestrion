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
#include "ScoreAttributionModel.h"

#include "log.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace dgk
{
ScoreAttributionModel::ScoreAttributionModel(QObject *parent) : QObject(parent)
{
}

void ScoreAttributionModel::load()
{
  loadManifest();

  context()->currentProjectChanged().onNotify(this, [this]
                                              { onCurrentProjectChanged(); });
}

void ScoreAttributionModel::loadManifest()
{
  const QString manifestPath =
      globalConfiguration()->appDataPath().toQString() +
      "scores/attributions.json";

  QFile file(manifestPath);
  if (!file.exists())
    // Not an error: scores simply may not need attribution.
    return;

  if (!file.open(QIODevice::ReadOnly))
  {
    LOGW() << "Could not open attribution manifest: " << manifestPath;
    return;
  }

  QJsonParseError error;
  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject())
  {
    LOGW() << "Could not parse attribution manifest: "
           << error.errorString().toStdString();
    return;
  }

  const QJsonObject root = doc.object();
  for (auto it = root.constBegin(); it != root.constEnd(); ++it)
  {
    if (!it.value().isObject())
      continue;
    const QJsonObject entry = it.value().toObject();
    const QString author = entry.value("author").toString();
    if (author.isEmpty())
      continue;
    m_attributions.insert(
        it.key(),
        Attribution{author, entry.value("license").toString(),
                    entry.value("url").toString()});
  }
}

void ScoreAttributionModel::onCurrentProjectChanged()
{
  m_current = Attribution{};

  if (const auto project = context()->currentProject())
  {
    const QString fileName =
        QFileInfo(project->path().toQString()).fileName();
    const auto it = m_attributions.constFind(fileName);
    if (it != m_attributions.constEnd())
      m_current = it.value();
  }

  emit attributionChanged();

  if (!m_current.author.isEmpty())
    emit attributionRequired();
}

void ScoreAttributionModel::openSourceUrl() const
{
  if (!m_current.url.isEmpty())
    interactive()->openUrl(QUrl(m_current.url));
}

QString ScoreAttributionModel::author() const { return m_current.author; }

QString ScoreAttributionModel::license() const { return m_current.license; }

QString ScoreAttributionModel::url() const { return m_current.url; }
} // namespace dgk
