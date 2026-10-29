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

#include "OrchestrionShell/IRollsLibrary.h"
#include "OrchestrionShell/internal/ScoreIncipitCache.h"

#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <global/iglobalconfiguration.h>
#include <interactive/iinteractive.h>
#include <interactive/iplatforminteractive.h>
#include <project/iprojectsession.h>
#include <project/irecentfilescontroller.h>

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "OrchestrionCommon/OrchestrionIoc.h"

namespace dgk
{
/**
 * One section of the score shelf: its scores, as cards — those that match
 * the search, if any, best match first.
 */
class ScoreListModel : public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  enum Role
  {
    TitleRole = Qt::UserRole + 1,
    ComposerRole,
    /** The incipit's image URL; empty until rendered, or if it can't be. */
    IncipitRole,
    /** Whether title, composer and incipit are final. */
    ReadyRole,
    PathRole,
    /** The path with the platform's separators, for display. */
    NativePathRole,
    DisplayNameOverrideRole,
    /** Whether it is the score that is open. */
    CurrentRole,
    /** Whether the file is on this computer (a score of Rolls may not be). */
    DownloadedRole,
    DownloadingRole,
    /** Whether its last download failed. */
    DownloadFailedRole,
    /** Whether Rolls has a newer version than the one downloaded. */
    UpdateAvailableRole,
  };

  struct Item
  {
    QString path;
    QString displayNameOverride;
    QString title;
    QString composer;
    QString incipit;
    bool ready = false;
    /** Title, composer and file name, folded for search. */
    QString searchText;
    /** For a score of Rolls: its id. */
    QString rollsId;
    /**
     * Whether title and composer come from the Rolls index, which the
     * score's own may not replace.
     */
    bool titledByIndex = false;
    bool downloaded = true;
    bool downloading = false;
    bool downloadFailed = false;
    bool updateAvailable = false;
  };

  explicit ScoreListModel(QObject *parent = nullptr);

  int count() const;
  int rowCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  /** All the section's items, whether shown or not. */
  const std::vector<Item> &items() const;
  void setItems(std::vector<Item> items);
  /** Fills in every item of that path. */
  void setEntry(const QString &path, const ScoreIncipitCache::Entry &entry);
  /** Sets the download state of every item of that score of Rolls. */
  void setDownloadState(const QString &rollsId, bool downloading, bool failed);
  void setCurrentPath(const QString &path);
  /** Shows only the items that match: a query as foldForSearch() returns. */
  void setQuery(const QString &foldedQuery);
  /** The first item shown and how well it matches the query, if any. */
  std::optional<std::pair<int, const Item *>> bestMatch() const;

signals:
  void countChanged();

private:
  /** Recomputes which items are shown, and in what order. */
  void filter();
  const Item &shownItem(int row) const;

  std::vector<Item> m_items;
  /** Indices into m_items of the items shown, in order. */
  std::vector<size_t> m_shown;
  /** How well the first item shown matches. */
  int m_bestScore = 0;
  QString m_query;
  QString m_currentPath;
};

/**
 * The shelf's sections, in the order they are shown: the recently played
 * scores, then each folder the user added, then the library: the scores
 * bundled with the application and those of Rolls.
 */
class ScoreSectionsModel : public QAbstractListModel
{
  Q_OBJECT

public:
  enum Role
  {
    KindRole = Qt::UserRole + 1,
    TitleRole,
    /**
     * The folder, for a folder's section ("~" for the home directory); the
     * address of Rolls, for the library's.
     */
    SubtitleRole,
    PathRole,
    ScoresRole,
  };

  enum class Kind
  {
    Recent,
    Folder,
    Library,
  };

  struct Section
  {
    Kind kind;
    QString title;
    QString path;
    ScoreListModel *scores;
  };

  explicit ScoreSectionsModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  const std::vector<Section> &sections() const;
  void setSections(std::vector<Section> sections);

private:
  std::vector<Section> m_sections;
};

/**
 * Backs the score shelf, the overlay that shows the scores one can open as
 * cards: the recently opened ones, those in the folders the user added and
 * the library (the bundled scores and those of Rolls, IRollsLibrary), each
 * with its title, composer and the beginning of its music. It opens over the
 * score when the score's title is clicked or from File ▸ Browse scores…
 * (openRequested()).
 *
 * A score is shown once, in the first section that has it.
 *
 * A score of Rolls is downloaded when it is first opened; until then its card
 * has no music. A newer version is downloaded only when the user asks for it
 * (update()), as it replaces the one they practised.
 *
 * The cards' music comes from a disk cache (ScoreIncipitCache). What is
 * missing from it is rendered while the shelf is shown, one score at a time
 * so that the shelf stays responsive in between, the cards in view first, and
 * never while it is hidden: rendering blocks the GUI thread, which mustn't
 * happen while the user is playing.
 */
class ScoreShelfModel : public QObject,
                        public dgk::Injectable,
                        public muse::async::Asyncable,
                        public muse::actions::Actionable
{
  Q_OBJECT

  Q_PROPERTY(QObject *sections READ sections CONSTANT)
  /** Whether the user added folders of their own. */
  Q_PROPERTY(bool hasFolders READ hasFolders NOTIFY foldersChanged)
  /** How many cards are shown, in all sections. */
  Q_PROPERTY(int shownCount READ shownCount NOTIFY shownCountChanged)
  /** Set while the shelf is shown: that is when missing cards get rendered. */
  Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
  /** What the search box holds: only the scores that match it are shown. */
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
  /** Whether a score is open (if not, the shelf can't be closed). */
  Q_PROPERTY(bool hasScore READ hasScore NOTIFY hasScoreChanged)

  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<mu::project::IProjectSession> projectSession{this};
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};
  dgk::Inject<mu::project::IRecentFilesController> recentFilesController{this};
  dgk::Inject<muse::IInteractive> interactive{this};
  dgk::Inject<muse::IPlatformInteractive> platformInteractive{this};
  dgk::Inject<IRollsLibrary> rollsLibrary{this};

public:
  explicit ScoreShelfModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();

  /**
   * Opens that score (in place of the current one). Returns false if it must
   * be downloaded first: it is then opened once it is, if it is still the one
   * chosen last, and closeRequested() is emitted.
   */
  Q_INVOKABLE bool open(const QString &path,
                        const QString &displayNameOverride);
  /**
   * Opens the score that best matches the search, of any section (Return in
   * the search box). Returns false if none does, or if it must be downloaded
   * first (see open()).
   */
  Q_INVOKABLE bool openBestMatch();
  /**
   * Asks whether to replace that score of Rolls with its newer version, and
   * downloads it if so.
   */
  Q_INVOKABLE void update(const QString &path);
  /** Rolls' website. */
  Q_INVOKABLE void openRollsSite();
  /** The shelf's links: the same as the File menu's. */
  Q_INVOKABLE void openFromComputer();
  Q_INVOKABLE void searchOnline();
  Q_INVOKABLE void helpFindScores();

  /** Asks for a folder and adds it to the shelf. */
  Q_INVOKABLE void addFolder();
  /**
   * What was dropped on the shelf: folders are added to it; a single score
   * is opened (closeRequested()). Returns whether anything was done.
   */
  Q_INVOKABLE bool drop(const QList<QUrl> &urls);
  /** Takes the folder off the shelf (not off the disk). */
  Q_INVOKABLE void removeFolder(const QString &path);
  Q_INVOKABLE void revealFolder(const QString &path);
  /** Shows the score's file in the file manager. */
  Q_INVOKABLE void revealScore(const QString &path);
  /**
   * The scores whose cards are in view, to be rendered before the others.
   */
  Q_INVOKABLE void prioritize(const QStringList &paths);

  QObject *sections() const;
  bool hasFolders() const;
  int shownCount() const;
  bool active() const;
  void setActive(bool active);
  bool hasScore() const;
  QString query() const;
  void setQuery(const QString &query);

signals:
  void openRequested();
  /** A score was opened from outside the cards (dropped on the shelf). */
  void closeRequested();
  void foldersChanged();
  void shownCountChanged();
  void activeChanged();
  void hasScoreChanged();
  void queryChanged();

private:
  /** The folders the user added, as saved in the settings. */
  QStringList folders() const;
  void setFolders(const QStringList &folders);
  /** Adds the folder unless it is there already; returns whether it was. */
  bool addFolderPath(const QString &path);

  /** Rebuilds every section, each leaving out what those above it have. */
  void reload();
  /** reload(), a little later: a folder's files change in bursts. */
  void scheduleReload();
  void watch(const QStringList &directories);
  void onCurrentProjectChanged();
  void onDownloadFinished(const QString &id, bool ok);
  ScoreListModel *makeList();
  ScoreListModel::Item makeItem(const QString &path,
                                const QString &displayNameOverride) const;
  /** Every section's list, in order. */
  std::vector<ScoreListModel *> lists() const;
  /** Renders a card still missing, then schedules the next one. */
  void renderNext();
  void scheduleRender();

  ScoreIncipitCache m_cache;
  ScoreSectionsModel *const m_sections;
  ScoreListModel *const m_recentScores;
  ScoreListModel *const m_libraryScores;
  /** The folders' lists, kept across reloads by folder path. */
  std::map<QString, ScoreListModel *> m_folderScores;
  QFileSystemWatcher *const m_watcher;
  QTimer *const m_reloadTimer;
  QStringList m_priority;
  QString m_query;
  QString m_currentPath;
  /** The score of Rolls to open once downloaded. */
  QString m_pendingOpen;
  QString m_pendingDisplayName;
  bool m_active = false;
  bool m_renderScheduled = false;
};
} // namespace dgk
