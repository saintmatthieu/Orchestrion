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
#include "ScoreShelfModel.h"
#include "MuseScoreShell/OrchestrionActionIds.h"
#include "OrchestrionCommon/BundledScores.h"
#include "OrchestrionShell/internal/FuzzySearch.h"
#include "OrchestrionShell/internal/ScoreTitles.h"

#include <async/async.h>
#include <global/settings.h>
#include <global/translation.h>
#include <project/inotationproject.h>

#include <QCollator>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QTimer>
#include <QUrl>
#include <algorithm>

namespace dgk
{
namespace
{
/** How many recently opened scores the shelf shows. */
constexpr int maxRecentScores = 8;

/**
 * The pause between two renderings, so that the shelf gets to paint the
 * card just rendered (and to answer the mouse) before the next one blocks.
 */
constexpr int renderPauseMs = 30;

/**
 * How long a folder's files must stay put before the shelf is reloaded: a
 * download or a copy changes them several times in a row.
 */
constexpr int reloadDelayMs = 500;

/**
 * How deep in a folder the shelf looks for scores, and how many it lists at
 * most: enough for a library of scores, bounded for when someone adds their
 * whole home folder.
 */
constexpr int maxFolderDepth = 3;
constexpr int maxFolderScores = 500;
constexpr int maxWatchedDirectories = 200;

const muse::Settings::Key SCORE_FOLDERS("OrchestrionShell", "SCORE_FOLDERS");

const QStringList scoreSuffixes{"mscz", "mscx", "mxl", "musicxml", "xml"};

struct FolderScan
{
  /** Sorted as people would: by their path in the folder, numbers by value. */
  QFileInfoList scores;
  /** The folder and its subfolders, to be watched. */
  QStringList directories;
};

FolderScan scanFolder(const QString &folder)
{
  FolderScan scan;
  const QDir root{folder};
  if (!root.exists())
    return scan;
  const auto depth = [&root](const QString &path)
  { return root.relativeFilePath(path).count('/'); };

  scan.directories << root.absolutePath();
  QDirIterator dirs{folder, QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories};
  while (dirs.hasNext() && scan.directories.size() < maxWatchedDirectories)
  {
    const QString dir = dirs.next();
    if (depth(dir) < maxFolderDepth)
      scan.directories << dir;
  }

  QStringList nameFilters;
  for (const QString &suffix : scoreSuffixes)
    nameFilters << "*." + suffix;
  QDirIterator files{folder, nameFilters, QDir::Files,
                     QDirIterator::Subdirectories};
  while (files.hasNext() && scan.scores.size() < maxFolderScores)
  {
    const QString file = files.next();
    if (depth(file) <= maxFolderDepth)
      scan.scores << QFileInfo{file};
  }

  QCollator collator;
  collator.setNumericMode(true);
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  std::sort(scan.scores.begin(), scan.scores.end(),
            [&](const QFileInfo &a, const QFileInfo &b)
            {
              return collator.compare(
                         root.relativeFilePath(a.absoluteFilePath()),
                         root.relativeFilePath(b.absoluteFilePath())) < 0;
            });
  return scan;
}

QString imageUrl(const QString &path)
{
  return path.isEmpty() ? QString{} : QUrl::fromLocalFile(path).toString();
}

// The score whose music the item's card shows
const QString &pictureOf(const ScoreListModel::Item &item)
{
  return item.tutorial ? item.scorePath : item.path;
}

QString searchText(const ScoreListModel::Item &item)
{
  // The file name too: it is what a user who downloaded the score knows it
  // by.
  return foldForSearch(item.title + ' ' + item.composer + ' ' +
                       titleFromFileName(muse::io::path_t{item.path}));
}
} // namespace

ScoreListModel::ScoreListModel(QObject *parent) : QAbstractListModel(parent) {}

int ScoreListModel::count() const { return static_cast<int>(m_shown.size()); }

int ScoreListModel::rowCount(const QModelIndex &parent) const
{
  return parent.isValid() ? 0 : count();
}

const ScoreListModel::Item &ScoreListModel::shownItem(int row) const
{
  return m_items[m_shown[static_cast<size_t>(row)]];
}

QVariant ScoreListModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() >= count())
    return {};
  const Item &item = shownItem(index.row());
  switch (role)
  {
  case TitleRole:
    return item.title;
  case ComposerRole:
    return item.composer;
  case IncipitRole:
    return item.incipit;
  case ReadyRole:
    return item.ready;
  case PathRole:
    return item.path;
  case NativePathRole:
    return QDir::toNativeSeparators(item.path);
  case DisplayNameOverrideRole:
    return item.displayNameOverride;
  case CurrentRole:
    return !m_currentPath.isEmpty() && item.path == m_currentPath;
  case DownloadedRole:
    return item.downloaded;
  case DownloadingRole:
    return item.downloading;
  case DownloadFailedRole:
    return item.downloadFailed;
  case UpdateAvailableRole:
    return item.updateAvailable;
  case TutorialRole:
    return item.tutorial;
  case PicturePathRole:
    return pictureOf(item);
  default:
    return {};
  }
}

QHash<int, QByteArray> ScoreListModel::roleNames() const
{
  return {
      {TitleRole, "title"},
      {ComposerRole, "composer"},
      {IncipitRole, "incipit"},
      {ReadyRole, "ready"},
      {PathRole, "path"},
      {NativePathRole, "nativePath"},
      {DisplayNameOverrideRole, "displayNameOverride"},
      {CurrentRole, "current"},
      {DownloadedRole, "downloaded"},
      {DownloadingRole, "downloading"},
      {DownloadFailedRole, "downloadFailed"},
      {UpdateAvailableRole, "updateAvailable"},
      {TutorialRole, "tutorial"},
      {PicturePathRole, "picturePath"},
  };
}

const std::vector<ScoreListModel::Item> &ScoreListModel::items() const
{
  return m_items;
}

void ScoreListModel::setItems(std::vector<Item> items)
{
  m_items = std::move(items);
  for (Item &item : m_items)
    item.searchText = searchText(item);
  filter();
}

void ScoreListModel::setEntry(const QString &path,
                              const ScoreIncipitCache::Entry &entry)
{
  bool found = false;
  for (Item &item : m_items)
  {
    if (pictureOf(item) != path)
      continue;
    if (!item.titledByIndex && !item.tutorial)
    {
      item.title = entry.title;
      item.composer = entry.composer;
    }
    item.incipit = imageUrl(entry.imagePath);
    item.ready = true;
    item.searchText = searchText(item);
    found = true;
  }
  if (!found)
    return;
  // The real title may match the search where the file name didn't, or the
  // other way round.
  if (!m_query.isEmpty())
  {
    filter();
    return;
  }
  for (int row = 0; row < count(); ++row)
    if (pictureOf(shownItem(row)) == path)
      emit dataChanged(index(row), index(row),
                       {TitleRole, ComposerRole, IncipitRole, ReadyRole});
}

void ScoreListModel::setDownloadState(const QString &rollsId, bool downloading,
                                      bool failed)
{
  for (Item &item : m_items)
    if (item.rollsId == rollsId)
    {
      item.downloading = downloading;
      item.downloadFailed = failed;
    }
  for (int row = 0; row < count(); ++row)
    if (shownItem(row).rollsId == rollsId)
      emit dataChanged(index(row), index(row),
                       {DownloadingRole, DownloadFailedRole});
}

void ScoreListModel::setCurrentPath(const QString &path)
{
  if (path == m_currentPath)
    return;
  m_currentPath = path;
  if (count() > 0)
    emit dataChanged(index(0), index(count() - 1), {CurrentRole});
}

void ScoreListModel::setQuery(const QString &foldedQuery)
{
  if (foldedQuery == m_query)
    return;
  m_query = foldedQuery;
  filter();
}

void ScoreListModel::filter()
{
  std::vector<std::pair<int, size_t>> matches; // (score, index)
  for (size_t i = 0; i < m_items.size(); ++i)
    if (const std::optional<int> score =
            fuzzyScore(m_query, m_items[i].searchText))
      matches.emplace_back(*score, i);
  // Without a query everything scores 0 and keeps its order.
  std::stable_sort(matches.begin(), matches.end(),
                   [](const auto &a, const auto &b)
                   { return a.first > b.first; });

  beginResetModel();
  m_bestScore = matches.empty() ? 0 : matches.front().first;
  m_shown.clear();
  for (const auto &[score, i] : matches)
    m_shown.push_back(i);
  endResetModel();
  emit countChanged();
}

std::optional<std::pair<int, const ScoreListModel::Item *>>
ScoreListModel::bestMatch() const
{
  if (m_shown.empty())
    return std::nullopt;
  return std::make_pair(m_bestScore, &shownItem(0));
}

ScoreSectionsModel::ScoreSectionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ScoreSectionsModel::rowCount(const QModelIndex &parent) const
{
  return parent.isValid() ? 0 : static_cast<int>(m_sections.size());
}

QVariant ScoreSectionsModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return {};
  const Section &section = m_sections[static_cast<size_t>(index.row())];
  switch (role)
  {
  case KindRole:
    switch (section.kind)
    {
    case Kind::Tutorials:
      return QStringLiteral("tutorials");
    case Kind::Recent:
      return QStringLiteral("recent");
    case Kind::Folder:
      return QStringLiteral("folder");
    case Kind::Library:
      return QStringLiteral("library");
    }
    return {};
  case TitleRole:
    return section.title;
  case SubtitleRole:
  {
    if (section.kind == Kind::Library)
      return section.path;
    if (section.kind != Kind::Folder)
      return QString{};
    const QString home = QDir::homePath();
    QString path = section.path;
    if (path == home || path.startsWith(home + '/'))
      path = '~' + path.mid(home.size());
    return QDir::toNativeSeparators(path);
  }
  case PathRole:
    return section.path;
  case ScoresRole:
    return QVariant::fromValue<QObject *>(section.scores);
  default:
    return {};
  }
}

QHash<int, QByteArray> ScoreSectionsModel::roleNames() const
{
  return {
      {KindRole, "kind"}, {TitleRole, "title"},   {SubtitleRole, "subtitle"},
      {PathRole, "path"}, {ScoresRole, "scores"},
  };
}

const std::vector<ScoreSectionsModel::Section> &
ScoreSectionsModel::sections() const
{
  return m_sections;
}

void ScoreSectionsModel::setSections(std::vector<Section> sections)
{
  beginResetModel();
  m_sections = std::move(sections);
  endResetModel();
}

ScoreShelfModel::ScoreShelfModel(QObject *parent)
    : QObject(parent), m_sections{new ScoreSectionsModel(this)},
      m_tutorialScores{makeList()}, m_recentScores{makeList()},
      m_libraryScores{makeList()}, m_watcher{new QFileSystemWatcher(this)},
      m_reloadTimer{new QTimer(this)}
{
  m_reloadTimer->setSingleShot(true);
  m_reloadTimer->setInterval(reloadDelayMs);
  connect(m_reloadTimer, &QTimer::timeout, this, [this] { reload(); });
  connect(m_watcher, &QFileSystemWatcher::directoryChanged, this,
          [this] { scheduleReload(); });
}

void ScoreShelfModel::load()
{
  muse::settings()->setDefaultValue(SCORE_FOLDERS, muse::Val{QString{}});

  reload();
  onCurrentProjectChanged();

  recentFilesController()->recentFilesListChanged().onReceive(
      this, [this](const mu::project::RecentFilesChange &) { reload(); });
  projectSession()->projectChanged().onNotify(this, [this]
                                              { onCurrentProjectChanged(); });
  rollsLibrary()->scoresChanged().onNotify(this, [this] { reload(); });
  rollsLibrary()->downloadFinished().onReceive(
      this, [this](const QString &id, bool ok) { onDownloadFinished(id, ok); });

  dispatcher()->reg(this, actionIds::browseScores,
                    [this] { emit openRequested(); });
  dispatcher()->reg(this, "orchestrion-tutorials",
                    [this] { emit tutorialsRequested(); });
}

QObject *ScoreShelfModel::sections() const { return m_sections; }

bool ScoreShelfModel::hasFolders() const { return !folders().isEmpty(); }

int ScoreShelfModel::shownCount() const
{
  int count = 0;
  for (const ScoreListModel *list : lists())
    count += list->count();
  return count;
}

int ScoreShelfModel::tutorialCount() const { return m_tutorialScores->count(); }

bool ScoreShelfModel::active() const { return m_active; }

void ScoreShelfModel::setActive(bool active)
{
  if (active == m_active)
    return;
  m_active = active;
  emit activeChanged();
  if (m_active)
  {
    // Not at startup: only someone looking for a score needs the library.
    rollsLibrary()->refresh();
    scheduleRender();
  }
}

QString ScoreShelfModel::query() const { return m_query; }

void ScoreShelfModel::setQuery(const QString &query)
{
  if (query == m_query)
    return;
  m_query = query;
  const QString folded = foldForSearch(query);
  for (ScoreListModel *list : lists())
    list->setQuery(folded);
  emit queryChanged();
}

bool ScoreShelfModel::hasScore() const
{
  return projectSession()->project() != nullptr;
}

bool ScoreShelfModel::open(const QString &path,
                           const QString &displayNameOverride)
{
  if (const auto id = rollsLibrary()->idOf(path);
      id && !rollsLibrary()->isDownloaded(*id))
  {
    m_pendingOpen = path;
    m_pendingDisplayName = displayNameOverride;
    rollsLibrary()->download(*id);
    for (ScoreListModel *list : lists())
      list->setDownloadState(*id, rollsLibrary()->isDownloading(*id), false);
    return false;
  }
  m_pendingOpen.clear();
  // Deferred: the click comes from the shelf, which lies in the score view,
  // whose model opening a score tears down and rebuilds.
  muse::async::Async::call(
      this,
      [this, path, displayNameOverride]
      {
        dispatcher()->dispatch(
            "orchestrion-file-open",
            muse::actions::ActionData::make_arg2<QUrl, QString>(
                QUrl::fromLocalFile(path), displayNameOverride));
      });
  return true;
}

bool ScoreShelfModel::openBestMatch()
{
  // The first section's on a tie: it's the one listed first.
  std::optional<std::pair<int, const ScoreListModel::Item *>> best;
  for (const ScoreListModel *list : lists())
    if (const auto match = list->bestMatch();
        match && (!best || match->first > best->first))
      best = match;
  if (!best)
    return false;
  if (best->second->tutorial)
  {
    runTutorial(best->second->path);
    return true;
  }
  return open(best->second->path, best->second->displayNameOverride);
}

void ScoreShelfModel::runTutorial(const QString &path)
{
  // Deferred, as open() is: the click comes from the shelf.
  muse::async::Async::call(this, [this, path] { tutorials()->run(path); });
}

void ScoreShelfModel::reloadTutorials()
{
  const QString bundledScore =
      globalConfiguration()->appDataPath().toQString() +
      "scores/Frere Jacques.mxl";
  const QString scorePath =
      m_currentPath.isEmpty() ? bundledScore : m_currentPath;
  std::vector<ScoreListModel::Item> items;
  for (const ITutorials::Tutorial &tutorial : tutorials()->tutorials())
  {
    ScoreListModel::Item item = makeItem(scorePath, QString{});
    item.path = tutorial.path;
    item.title = tutorial.title;
    item.composer = tutorial.description;
    item.tutorial = true;
    item.scorePath = scorePath;
    items.push_back(std::move(item));
  }
  m_tutorialScores->setItems(std::move(items));
}

void ScoreShelfModel::update(const QString &path)
{
  const auto id = rollsLibrary()->idOf(path);
  if (!id || !rollsLibrary()->hasUpdate(*id))
    return;
  const RollsScore *const score = rollsLibrary()->score(*id);
  using muse::IInteractive;
  constexpr int updateButton = int(IInteractive::Button::CustomButton) + 1;
  const IInteractive::Result result = interactive()->questionSync(
      muse::qtrc("orchestrion/shelf", "Update “%1”?")
          .arg(score->title)
          .toStdString(),
      muse::trc("orchestrion/shelf",
                "A newer version of this score is available. It replaces the "
                "one on this computer, with any changes you saved to it, and "
                "may differ from what you practised: the takes you recorded "
                "were played on the version you have."),
      IInteractive::ButtonDatas{
          IInteractive::ButtonData{IInteractive::Button::Cancel,
                                   muse::trc("orchestrion/shelf", "Not now")},
          IInteractive::ButtonData{
              updateButton, muse::trc("orchestrion/shelf", "Update"), true}},
      int(IInteractive::Button::Cancel));
  if (result.button() != updateButton)
    return;
  rollsLibrary()->download(*id);
  for (ScoreListModel *list : lists())
    list->setDownloadState(*id, rollsLibrary()->isDownloading(*id), false);
}

void ScoreShelfModel::openRollsSite()
{
  platformInteractive()->openUrl(rollsLibrary()->siteUrl());
}

void ScoreShelfModel::onDownloadFinished(const QString &id, bool ok)
{
  const QString path = rollsLibrary()->localPath(id);
  if (!ok)
  {
    if (m_pendingOpen == path)
      m_pendingOpen.clear();
    for (ScoreListModel *list : lists())
      list->setDownloadState(id, false, true);
    return;
  }
  // The card now has its music to draw, or a new version of it.
  reload();
  if (m_pendingOpen == path)
  {
    m_pendingOpen.clear();
    open(path, m_pendingDisplayName);
    emit closeRequested();
  }
}

void ScoreShelfModel::openFromComputer()
{
  dispatcher()->dispatch("orchestrion-file-open");
}

void ScoreShelfModel::searchOnline()
{
  dispatcher()->dispatch("orchestrion-search-musescore");
}

void ScoreShelfModel::helpFindScores()
{
  dispatcher()->dispatch("orchestrion-file-help");
}

void ScoreShelfModel::addFolder()
{
  const QStringList current = folders();
  const muse::io::path_t start{current.isEmpty() ? QDir::homePath()
                                                 : current.back()};
  const muse::io::path_t chosen = interactive()->selectDirectory(
      muse::trc("orchestrion/shelf", "Add a folder of scores"), start);
  if (!chosen.empty())
    addFolderPath(chosen.toQString());
}

bool ScoreShelfModel::drop(const QList<QUrl> &urls)
{
  bool added = false;
  QStringList scores;
  for (const QUrl &url : urls)
  {
    const QFileInfo file{url.toLocalFile()};
    if (file.isDir())
      added = addFolderPath(file.absoluteFilePath()) || added;
    else if (file.isFile() &&
             scoreSuffixes.contains(file.suffix(), Qt::CaseInsensitive))
      scores << file.absoluteFilePath();
  }
  if (added)
    return true;
  if (scores.size() == 1)
  {
    open(scores.front(), QString{});
    emit closeRequested();
    return true;
  }
  return false;
}

void ScoreShelfModel::removeFolder(const QString &path)
{
  QStringList current = folders();
  if (current.removeAll(path) > 0)
    setFolders(current);
}

void ScoreShelfModel::revealFolder(const QString &path)
{
  platformInteractive()->openUrl(QUrl::fromLocalFile(path));
}

void ScoreShelfModel::revealScore(const QString &path)
{
  platformInteractive()->revealInFileBrowser(muse::io::path_t{path});
}

void ScoreShelfModel::prioritize(const QStringList &paths)
{
  m_priority = paths;
  scheduleRender();
}

QStringList ScoreShelfModel::folders() const
{
  QStringList result;
  const QJsonArray array =
      QJsonDocument::fromJson(
          muse::settings()->value(SCORE_FOLDERS).toQString().toUtf8())
          .array();
  for (const QJsonValue &value : array)
    result << value.toString();
  return result;
}

void ScoreShelfModel::setFolders(const QStringList &folders)
{
  muse::settings()->setSharedValue(
      SCORE_FOLDERS,
      muse::Val{QString::fromUtf8(
          QJsonDocument{QJsonArray::fromStringList(folders)}.toJson(
              QJsonDocument::Compact))});
  emit foldersChanged();
  reload();
}

bool ScoreShelfModel::addFolderPath(const QString &path)
{
  const QString cleaned = QDir::cleanPath(path);
  QStringList current = folders();
  if (current.contains(cleaned))
    return false;
  current << cleaned;
  setFolders(current);
  return true;
}

ScoreListModel *ScoreShelfModel::makeList()
{
  auto *const list = new ScoreListModel(this);
  connect(list, &ScoreListModel::countChanged, this,
          &ScoreShelfModel::shownCountChanged);
  return list;
}

ScoreListModel::Item
ScoreShelfModel::makeItem(const QString &path,
                          const QString &displayNameOverride) const
{
  ScoreListModel::Item item{path, displayNameOverride,
                            titleFromFileName(muse::io::path_t{path})};
  if (const auto entry = m_cache.cached(path))
  {
    item.title = entry->title;
    item.composer = entry->composer;
    item.incipit = imageUrl(entry->imagePath);
    item.ready = true;
  }
  const auto id = rollsLibrary()->idOf(path);
  if (!id)
    return item;
  item.rollsId = *id;
  if (const RollsScore *const score = rollsLibrary()->score(*id))
  {
    item.title = score->title;
    item.composer = score->composer;
    item.titledByIndex = true;
  }
  item.downloaded = rollsLibrary()->isDownloaded(*id);
  // Nothing to draw until it is downloaded.
  if (!item.downloaded)
    item.ready = true;
  item.downloading = rollsLibrary()->isDownloading(*id);
  item.updateAvailable = rollsLibrary()->hasUpdate(*id);
  return item;
}

std::vector<ScoreListModel *> ScoreShelfModel::lists() const
{
  std::vector<ScoreListModel *> result;
  for (const ScoreSectionsModel::Section &section : m_sections->sections())
    result.push_back(section.scores);
  return result;
}

void ScoreShelfModel::scheduleReload() { m_reloadTimer->start(); }

void ScoreShelfModel::watch(const QStringList &directories)
{
  if (const QStringList watched = m_watcher->directories(); !watched.isEmpty())
    m_watcher->removePaths(watched);
  if (!directories.isEmpty())
    m_watcher->addPaths(directories);
}

void ScoreShelfModel::reload()
{
  // By name and size rather than path: each build or install has its own
  // copy of the bundled scores, and one played recently may be another's;
  // a score copied into several folders is the same score. A score of Rolls
  // by its id: its file may not be there yet, or changed by a save.
  QSet<QString> seen;
  QSet<QString> seenRolls;
  const auto firstSeen = [&](const QFileInfo &file)
  {
    if (const auto id = rollsLibrary()->idOf(file.absoluteFilePath()))
    {
      if (seenRolls.contains(*id))
        return false;
      seenRolls.insert(*id);
      return true;
    }
    const QString identity =
        file.fileName() + '|' + QString::number(file.size());
    if (seen.contains(identity))
      return false;
    seen.insert(identity);
    return true;
  };

  std::vector<ScoreSectionsModel::Section> sections;

  reloadTutorials();
  sections.push_back({ScoreSectionsModel::Kind::Tutorials,
                      muse::qtrc("orchestrion/shelf", "Tutorials"), QString{},
                      m_tutorialScores});

  std::vector<ScoreListModel::Item> recent;
  for (const mu::project::RecentFile &file :
       recentFilesController()->recentFilesList())
  {
    if (static_cast<int>(recent.size()) == maxRecentScores)
      break;
    const QFileInfo info{file.path.toQString()};
    // A score of Rolls whose file went is back in the library: MuseScore
    // drops missing files from its list.
    if (info.exists() && firstSeen(info))
      recent.push_back(makeItem(info.filePath(), file.displayNameOverride));
  }
  m_recentScores->setItems(std::move(recent));
  sections.push_back({ScoreSectionsModel::Kind::Recent,
                      muse::qtrc("orchestrion/shelf", "Play again"), QString{},
                      m_recentScores});

  const QStringList userFolders = folders();
  QStringList watched;
  std::map<QString, ScoreListModel *> folderScores;
  for (const QString &folder : userFolders)
  {
    ScoreListModel *&list = folderScores[folder];
    if (const auto kept = m_folderScores.find(folder);
        kept != m_folderScores.end())
    {
      list = kept->second;
      m_folderScores.erase(kept);
    }
    else
    {
      list = makeList();
      list->setQuery(foldForSearch(m_query));
    }
    const FolderScan scan = scanFolder(folder);
    watched << scan.directories;
    std::vector<ScoreListModel::Item> items;
    for (const QFileInfo &file : scan.scores)
      if (firstSeen(file))
        items.push_back(makeItem(file.absoluteFilePath(), QString{}));
    list->setItems(std::move(items));
    const QString name = QFileInfo{folder}.fileName();
    sections.push_back({ScoreSectionsModel::Kind::Folder,
                        name.isEmpty() ? folder : name, folder, list});
  }
  // Those of the folders taken off the shelf.
  for (const auto &[folder, list] : m_folderScores)
    list->deleteLater();
  m_folderScores = std::move(folderScores);
  watch(watched);

  std::vector<ScoreListModel::Item> library;
  for (const QFileInfo &file : bundledScores(
           globalConfiguration()->appDataPath().toQString() + "scores/"))
    if (firstSeen(file))
      library.push_back(makeItem(file.absoluteFilePath(), QString{}));
  for (const RollsScore &score : rollsLibrary()->scores())
  {
    const QFileInfo file{rollsLibrary()->localPath(score.id)};
    if (firstSeen(file))
      library.push_back(makeItem(file.absoluteFilePath(), QString{}));
  }
  m_libraryScores->setItems(std::move(library));
  const QUrl site = rollsLibrary()->siteUrl();
  sections.push_back(
      {ScoreSectionsModel::Kind::Library,
       muse::qtrc("orchestrion/shelf", "Library"),
       site.host() + site.path().chopped(site.path().endsWith('/') ? 1 : 0),
       m_libraryScores});

  for (const ScoreSectionsModel::Section &section : sections)
    section.scores->setCurrentPath(m_currentPath);
  m_sections->setSections(std::move(sections));
  emit shownCountChanged();
  scheduleRender();
}

void ScoreShelfModel::onCurrentProjectChanged()
{
  const auto project = projectSession()->project();
  m_currentPath = project ? project->path().toQString() : QString{};
  for (ScoreListModel *list : lists())
    list->setCurrentPath(m_currentPath);
  // Their cards show the music they would play.
  reloadTutorials();
  scheduleRender();
  emit hasScoreChanged();
}

void ScoreShelfModel::scheduleRender()
{
  if (!m_active || m_renderScheduled)
    return;
  m_renderScheduled = true;
  QTimer::singleShot(renderPauseMs, this,
                     [this]
                     {
                       m_renderScheduled = false;
                       renderNext();
                     });
}

void ScoreShelfModel::renderNext()
{
  if (!m_active)
    return;
  const std::vector<ScoreListModel *> all = lists();
  const auto missing = [&all](const QString &path)
  {
    for (const ScoreListModel *list : all)
      for (const ScoreListModel::Item &item : list->items())
        if (pictureOf(item) == path && !item.ready)
          return true;
    return false;
  };

  // The cards in view first, then the others in the shelf's order.
  QString path;
  while (!m_priority.isEmpty() && path.isEmpty())
  {
    const QString candidate = m_priority.takeFirst();
    if (missing(candidate))
      path = candidate;
  }
  for (const ScoreListModel *list : all)
  {
    if (!path.isEmpty())
      break;
    for (const ScoreListModel::Item &item : list->items())
      if (!item.ready)
      {
        path = pictureOf(item);
        break;
      }
  }
  if (path.isEmpty())
    return;

  const ScoreIncipitCache::Entry entry = m_cache.render(path);
  for (ScoreListModel *list : all)
    list->setEntry(path, entry);
  scheduleRender();
}
} // namespace dgk
