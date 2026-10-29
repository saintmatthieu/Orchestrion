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

#include <async/channel.h>
#include <async/notification.h>
#include <modularity/imoduleinterface.h>

#include <QString>
#include <QUrl>
#include <optional>
#include <vector>

namespace dgk
{
/**
 * A score of Rolls (https://rolls.orchestrion.app/), as its index lists it.
 */
struct RollsScore
{
  /** A slug, never renamed: the local file is named after it. */
  QString id;
  QString title;
  /** The composer's full name. */
  QString composer;
  /** The credits, e.g. "Engraved for the Mutopia Project by …", joined. */
  QString credits;
  /** The names of those credited, joined, e.g. "Renato Biolcati Rinaldi". */
  QString creditedNames;
  QString licenseName;
  QString licenseUrl;
  /** The edition's page at its source (e.g. Mutopia's). */
  QString sourceUrl;
  QUrl musicxmlUrl;
  /** A hash of the MusicXML: a new version has a new one. */
  QString version;
};

/** Whether that is how the id of a score of Rolls can be written. */
bool isRollsId(const QString &id);

/**
 * The scores of Rolls, the online library of free scores made for
 * Orchestrion, and the copies of them downloaded to this computer.
 *
 * The index is kept on disk, so that the scores are listed offline; it is
 * fetched again by refresh(). A score is downloaded when it is first opened,
 * into the user's application data (not their documents: it is Orchestrion
 * that manages these files). A newer version is never downloaded unasked:
 * someone who practised a score must find it as they left it.
 */
class IRollsLibrary : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IRollsLibrary);

public:
  virtual ~IRollsLibrary() = default;

  /** The website, where the index is. */
  virtual QUrl siteUrl() const = 0;

  /** The scores of the last index fetched, in its order. */
  virtual const std::vector<RollsScore> &scores() const = 0;
  virtual const RollsScore *score(const QString &id) const = 0;
  virtual muse::async::Notification scoresChanged() const = 0;

  /**
   * Fetches the index again, unless it was a short while ago or a fetch is
   * under way. A failure keeps the scores as they were.
   */
  virtual void refresh() = 0;
  /**
   * Fetches the index again however recently it was, unless a fetch is under
   * way: for a score that may have been added since.
   */
  virtual void refreshNow() = 0;
  /**
   * A fetch of the index ended: whether Rolls could be reached. If it was,
   * the scores are those of the index fetched, unless this version of
   * Orchestrion can't read it.
   */
  virtual muse::async::Channel<bool> indexFetched() const = 0;

  /** Where the score is (or will be) downloaded. */
  virtual QString localPath(const QString &id) const = 0;
  /** The score's id if that is where a score of Rolls is downloaded. */
  virtual std::optional<QString> idOf(const QString &path) const = 0;
  virtual bool isDownloaded(const QString &id) const = 0;
  /** Whether the index has a newer version than the one downloaded. */
  virtual bool hasUpdate(const QString &id) const = 0;

  /**
   * Downloads the score's current version, replacing the one there is, if
   * any. Does nothing while it is being downloaded.
   */
  virtual void download(const QString &id) = 0;
  virtual bool isDownloading(const QString &id) const = 0;
  /** A download ended: the score's id and whether it succeeded. */
  virtual muse::async::Channel<QString, bool> downloadFinished() const = 0;
};
} // namespace dgk
