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
#include "RollsLibrary.h"

#include <log.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSaveFile>

namespace dgk
{
namespace
{
const QString defaultIndexUrl = "https://rolls.orchestrion.app/index.json";

/** The index's schema this version reads; a newer one is ignored. */
constexpr int schemaVersion = 1;

/**
 * How long a fetched index is used before it is asked for again, and how
 * long after a failed attempt.
 */
constexpr qint64 refreshIntervalSecs = 3 * 60 * 60;
constexpr qint64 retryIntervalSecs = 10 * 60;

constexpr int transferTimeoutMs = 30000;

const QRegularExpression idPattern{"^[a-z0-9]+(-[a-z0-9]+)*$"};

QUrl indexUrlFromEnvironment()
{
  const QByteArray url = qgetenv("ORCHESTRION_ROLLS_URL");
  return QUrl{url.isEmpty() ? defaultIndexUrl : QString::fromUtf8(url)};
}

QNetworkRequest makeRequest(const QUrl &url)
{
  QNetworkRequest request{url};
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    QCoreApplication::applicationName() + '/' +
                        QCoreApplication::applicationVersion());
  request.setTransferTimeout(transferTimeoutMs);
  return request;
}

bool writeFile(const QString &path, const QByteArray &data)
{
  QDir{}.mkpath(QFileInfo{path}.absolutePath());
  // Whole or not at all: a score half written would be opened as it is.
  QSaveFile file{path};
  if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() ||
      !file.commit())
  {
    LOGW() << "Could not write " << path << ": " << file.errorString();
    return false;
  }
  return true;
}

QString joined(const QJsonArray &credits, bool namesOnly)
{
  QStringList parts;
  for (const QJsonValue &value : credits)
  {
    const QJsonObject credit = value.toObject();
    const QString name = credit.value("name").toString();
    if (name.isEmpty())
      continue;
    const QString role = credit.value("role").toString();
    parts << (namesOnly || role.isEmpty() ? name : role + ' ' + name);
  }
  return parts.join(namesOnly ? ", " : "; ");
}
} // namespace

RollsLibrary::RollsLibrary() : m_indexUrl{indexUrlFromEnvironment()} {}

RollsLibrary::~RollsLibrary() = default;

QString RollsLibrary::directory() const
{
  return globalConfiguration()->userAppDataPath().toQString() + "/rolls";
}

QString RollsLibrary::scoresDirectory() const
{
  return directory() + "/scores";
}

QString RollsLibrary::indexPath() const { return directory() + "/index.json"; }

QString RollsLibrary::downloadedPath() const
{
  return directory() + "/downloaded.json";
}

void RollsLibrary::ensureLoaded() const
{
  if (m_loaded)
    return;
  m_loaded = true;

  if (QFile index{indexPath()}; index.open(QIODevice::ReadOnly))
    parseIndex(index.readAll(), m_indexUrl);

  if (QFile downloaded{downloadedPath()}; downloaded.open(QIODevice::ReadOnly))
  {
    const QJsonObject versions =
        QJsonDocument::fromJson(downloaded.readAll()).object();
    for (auto it = versions.begin(); it != versions.end(); ++it)
      m_downloaded.insert(it.key(), it.value().toString());
  }
}

bool RollsLibrary::parseIndex(const QByteArray &json,
                              const QUrl &indexUrl) const
{
  QJsonParseError error;
  const QJsonDocument doc = QJsonDocument::fromJson(json, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject())
  {
    LOGW() << "Could not parse the Rolls index: "
           << error.errorString().toStdString();
    return false;
  }
  const QJsonObject root = doc.object();
  if (root.value("schemaVersion").toInt() != schemaVersion)
  {
    LOGW() << "Rolls index of an unknown schema: "
           << root.value("schemaVersion").toInt();
    return false;
  }

  std::vector<RollsScore> scores;
  for (const QJsonValue &value : root.value("scores").toArray())
  {
    const QJsonObject entry = value.toObject();
    RollsScore score;
    score.id = entry.value("id").toString();
    const QString musicxml = entry.value("musicxml").toString();
    // The id names a file: nothing else than what the index promises.
    if (!idPattern.match(score.id).hasMatch() || musicxml.isEmpty())
      continue;
    score.title = entry.value("title").toString();
    score.composer = entry.value("composer").toString();
    const QJsonArray credits = entry.value("credits").toArray();
    score.credits = joined(credits, false);
    score.creditedNames = joined(credits, true);
    const QJsonObject license = entry.value("license").toObject();
    score.licenseName = license.value("name").toString();
    score.licenseUrl = license.value("url").toString();
    score.sourceUrl = entry.value("source").toObject().value("url").toString();
    // Relative to the index, wherever that is served from.
    score.musicxmlUrl = indexUrl.resolved(QUrl{musicxml});
    score.version = entry.value("version").toString();
    scores.push_back(std::move(score));
  }
  m_scores = std::move(scores);
  return true;
}

void RollsLibrary::saveDownloaded() const
{
  QJsonObject versions;
  for (auto it = m_downloaded.begin(); it != m_downloaded.end(); ++it)
    versions.insert(it.key(), it.value());
  writeFile(downloadedPath(), QJsonDocument{versions}.toJson());
}

QUrl RollsLibrary::siteUrl() const { return m_indexUrl.resolved(QUrl{"./"}); }

const std::vector<RollsScore> &RollsLibrary::scores() const
{
  ensureLoaded();
  return m_scores;
}

const RollsScore *RollsLibrary::score(const QString &id) const
{
  ensureLoaded();
  const auto it =
      std::find_if(m_scores.begin(), m_scores.end(),
                   [&id](const RollsScore &score) { return score.id == id; });
  return it == m_scores.end() ? nullptr : &*it;
}

muse::async::Notification RollsLibrary::scoresChanged() const
{
  return m_scoresChanged;
}

QNetworkAccessManager &RollsLibrary::network()
{
  if (!m_network)
    m_network = std::make_unique<QNetworkAccessManager>();
  return *m_network;
}

void RollsLibrary::refresh()
{
  ensureLoaded();
  if (m_fetching)
    return;
  const QDateTime now = QDateTime::currentDateTimeUtc();
  // Across sessions too: the index on disk says when it was fetched.
  const QDateTime fetched = QFileInfo{indexPath()}.lastModified().toUTC();
  if (fetched.isValid() && fetched.secsTo(now) < refreshIntervalSecs)
    return;
  if (m_lastAttempt.isValid() && m_lastAttempt.secsTo(now) < retryIntervalSecs)
    return;

  m_fetching = true;
  m_lastAttempt = now;
  QNetworkReply *const reply = network().get(makeRequest(m_indexUrl));
  QObject::connect(reply, &QNetworkReply::finished,
                   [this, reply] { onIndexFetched(reply); });
}

void RollsLibrary::onIndexFetched(QNetworkReply *reply)
{
  reply->deleteLater();
  m_fetching = false;
  if (reply->error() != QNetworkReply::NoError)
  {
    // Offline, most likely: the scores stay as they were.
    LOGI() << "Could not fetch the Rolls index: "
           << reply->errorString().toStdString();
    return;
  }
  const QByteArray json = reply->readAll();
  if (!parseIndex(json, m_indexUrl))
    return;
  writeFile(indexPath(), json);
  m_scoresChanged.notify();
}

QString RollsLibrary::localPath(const QString &id) const
{
  return scoresDirectory() + '/' + id + ".mxl";
}

std::optional<QString> RollsLibrary::idOf(const QString &path) const
{
  const QFileInfo file{path};
  if (file.suffix() != "mxl" || QDir::cleanPath(file.absolutePath()) !=
                                    QDir::cleanPath(scoresDirectory()))
    return std::nullopt;
  const QString id = file.completeBaseName();
  if (!idPattern.match(id).hasMatch())
    return std::nullopt;
  return id;
}

bool RollsLibrary::isDownloaded(const QString &id) const
{
  return QFileInfo::exists(localPath(id));
}

bool RollsLibrary::hasUpdate(const QString &id) const
{
  ensureLoaded();
  const RollsScore *const current = score(id);
  if (!current || !isDownloaded(id))
    return false;
  const auto downloaded = m_downloaded.constFind(id);
  return downloaded != m_downloaded.constEnd() &&
         *downloaded != current->version;
}

void RollsLibrary::download(const QString &id)
{
  const RollsScore *const wanted = score(id);
  if (!wanted || m_downloading.contains(id))
    return;
  m_downloading.insert(id);
  QNetworkReply *const reply = network().get(makeRequest(wanted->musicxmlUrl));
  // The version of the index at the time: it may be fetched again meanwhile.
  const QString version = wanted->version;
  QObject::connect(reply, &QNetworkReply::finished, [this, reply, id, version]
                   { onScoreFetched(reply, id, version); });
}

void RollsLibrary::onScoreFetched(QNetworkReply *reply, const QString &id,
                                  const QString &version)
{
  reply->deleteLater();
  m_downloading.remove(id);
  bool ok = reply->error() == QNetworkReply::NoError;
  if (!ok)
    LOGW() << "Could not download " << id << ": "
           << reply->errorString().toStdString();
  if (ok)
  {
    const QByteArray data = reply->readAll();
    ok = !data.isEmpty() && writeFile(localPath(id), data);
  }
  if (ok)
  {
    m_downloaded.insert(id, version);
    saveDownloaded();
  }
  m_downloadFinished.send(id, ok);
}

bool RollsLibrary::isDownloading(const QString &id) const
{
  return m_downloading.contains(id);
}

muse::async::Channel<QString, bool> RollsLibrary::downloadFinished() const
{
  return m_downloadFinished;
}
} // namespace dgk
