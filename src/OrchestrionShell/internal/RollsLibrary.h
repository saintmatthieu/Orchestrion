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

#include "IRollsLibrary.h"

#include <global/iglobalconfiguration.h>

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <memory>

#include "OrchestrionCommon/OrchestrionIoc.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace dgk
{
/**
 * IRollsLibrary over HTTPS, with its files in `<user app data>/rolls/`:
 * `index.json` (the last index fetched, as it was), `scores/<id>.mxl` and
 * `downloaded.json`, which says what version of each score was downloaded.
 *
 * The index's address can be changed with the ORCHESTRION_ROLLS_URL
 * environment variable, for testing against a local copy of the site.
 */
class RollsLibrary : public IRollsLibrary, public dgk::Injectable
{
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};

public:
  RollsLibrary();
  ~RollsLibrary() override;

  QUrl siteUrl() const override;
  const std::vector<RollsScore> &scores() const override;
  const RollsScore *score(const QString &id) const override;
  muse::async::Notification scoresChanged() const override;

  void refresh() override;
  void refreshNow() override;
  muse::async::Channel<bool> indexFetched() const override;

  QString localPath(const QString &id) const override;
  std::optional<QString> idOf(const QString &path) const override;
  bool isDownloaded(const QString &id) const override;
  bool hasUpdate(const QString &id) const override;

  void download(const QString &id) override;
  bool isDownloading(const QString &id) const override;
  muse::async::Channel<QString, bool> downloadFinished() const override;

private:
  QString directory() const;
  QString scoresDirectory() const;
  QString indexPath() const;
  QString downloadedPath() const;

  /** Reads what is on disk, the first time anything is asked. */
  void ensureLoaded() const;
  /** Parses an index; false, leaving the scores as they were, if it isn't
   * one this version of Orchestrion reads. */
  bool parseIndex(const QByteArray &json, const QUrl &indexUrl) const;
  void saveDownloaded() const;
  QNetworkAccessManager &network();

  void fetchIndex();
  void onIndexFetched(QNetworkReply *reply);
  void onScoreFetched(QNetworkReply *reply, const QString &id,
                      const QString &version);

  std::unique_ptr<QNetworkAccessManager> m_network;
  const QUrl m_indexUrl;

  // Loaded lazily, from const accessors.
  mutable bool m_loaded = false;
  mutable std::vector<RollsScore> m_scores;
  /** The version of each score downloaded, by id. */
  mutable QHash<QString, QString> m_downloaded;

  bool m_fetching = false;
  /** When the index was last asked for, whether that worked or not. */
  QDateTime m_lastAttempt;
  QSet<QString> m_downloading;

  muse::async::Notification m_scoresChanged;
  muse::async::Channel<bool> m_indexFetched;
  muse::async::Channel<QString, bool> m_downloadFinished;
};
} // namespace dgk
