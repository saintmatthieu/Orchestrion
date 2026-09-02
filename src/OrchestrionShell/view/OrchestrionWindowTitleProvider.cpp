/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "OrchestrionWindowTitleProvider.h"
#include <notation/inotation.h>
#include <project/inotationproject.h>
#include <project/types/projectmeta.h>
#include "OrchestrionSequencer/IModifiableItemRegistry.h"
#include "OrchestrionShell/internal/MuseScorePlaceholderMetaTags.h"
#include "io/path.h"
#include "translation.h"

#include <QStringList>
#include <algorithm>
#include <iterator>

namespace dgk
{
namespace
{
//! Whether `text` is one of MuseScore's new-score placeholder texts (in any
//! language), i.e. a meta tag the author never actually filled in.
template <size_t N>
bool isPlaceholder(const QString &text,
                   const char16_t *const (&placeholders)[N])
{
  return std::any_of(std::begin(placeholders), std::end(placeholders),
                     [&](const char16_t *placeholder)
                     {
                       return text.compare(QString::fromUtf16(placeholder),
                                           Qt::CaseInsensitive) == 0;
                     });
}
} // namespace

OrchestrionWindowTitleProvider::OrchestrionWindowTitleProvider(QObject *parent)
    : QObject(parent)
{
}

void OrchestrionWindowTitleProvider::load()
{
  update();

  context()->currentProjectChanged().onNotify(
      this,
      [this]()
      {
        if (auto currentProject = context()->currentProject())
        {
          currentProject->displayNameChanged().onNotify(this,
                                                        [this]() { update(); });

          currentProject->needSaveChanged().onNotify(this, [this]()
                                                    { update(); });
        }
      });

  context()->currentNotationChanged().onNotify(this, [this]() { update(); });

  orchestrion()->sequencerChanged().onNotify(
      this,
      [this]()
      {
        if (const auto registry = orchestrion()->modifiableItemRegistry())
          registry->ModifiedChanged().onNotify(this, [this]() { update(); });
      });
}

QString OrchestrionWindowTitleProvider::title() const { return m_title; }

QString OrchestrionWindowTitleProvider::scoreTitle() const
{
  return m_scoreTitle;
}

QString OrchestrionWindowTitleProvider::scoreComposer() const
{
  return m_scoreComposer;
}

QString OrchestrionWindowTitleProvider::filePath() const { return m_filePath; }

bool OrchestrionWindowTitleProvider::fileModified() const
{
  return m_fileModified;
}

void OrchestrionWindowTitleProvider::setTitle(const QString &title)
{
  if (title == m_title)
  {
    return;
  }

  m_title = title;
  emit titleChanged(title);
}

void OrchestrionWindowTitleProvider::setScoreTitle(const QString &scoreTitle)
{
  if (scoreTitle == m_scoreTitle)
  {
    return;
  }

  m_scoreTitle = scoreTitle;
  emit scoreTitleChanged(scoreTitle);
}

void OrchestrionWindowTitleProvider::setScoreComposer(
    const QString &scoreComposer)
{
  if (scoreComposer == m_scoreComposer)
  {
    return;
  }

  m_scoreComposer = scoreComposer;
  emit scoreComposerChanged(scoreComposer);
}

void OrchestrionWindowTitleProvider::setFilePath(const QString &filePath)
{
  if (filePath == m_filePath)
  {
    return;
  }

  m_filePath = filePath;
  emit filePathChanged(filePath);
}

void OrchestrionWindowTitleProvider::setFileModified(bool fileModified)
{
  if (fileModified == m_fileModified)
  {
    return;
  }

  m_fileModified = fileModified;
  emit fileModifiedChanged(fileModified);
}

void OrchestrionWindowTitleProvider::update()
{
  const mu::project::INotationProjectPtr project = context()->currentProject();
  const auto notation = context()->currentNotation();

  if (!project || !notation)
  {
    setTitle(muse::qtrc("appshell", "Orchestrion"));
    setScoreTitle("");
    setScoreComposer("");
    setFilePath("");
    setFileModified(false);
    return;
  }

  auto title = notation->projectNameAndPartName();
  if (const auto registry = orchestrion()->modifiableItemRegistry();
      registry->Modified())
    title += " *";
  setTitle(title);
  const mu::project::ProjectMeta meta = project->metaInfo();
  QString scoreTitle = meta.title.simplified();
  if (isPlaceholder(scoreTitle, musescore_placeholders::titles))
    scoreTitle.clear();
  if (scoreTitle.isEmpty())
  {
    // Untitled score: fall back to the file name, made presentable — no
    // extension, and the underscores that stand for spaces in downloaded
    // scores turned back into spaces.
    scoreTitle = muse::io::completeBasename(project->path())
                     .toQString()
                     .replace('_', ' ')
                     .simplified();
  }
  if (scoreTitle.isEmpty())
    scoreTitle = project->displayName();
  setScoreTitle(scoreTitle);
  // Multi-line tags ("Music: X\nArrangement: Y") go on the one line the
  // ornament offers, separated rather than run together.
  QStringList composerLines;
  for (const QString &line : meta.composer.split('\n'))
    if (const QString simplified = line.simplified();
        !simplified.isEmpty() &&
        !isPlaceholder(simplified, musescore_placeholders::composers))
      composerLines << simplified;
  setScoreComposer(composerLines.join(QStringLiteral(" \u00B7 ")));

  setFilePath((project->isNewlyCreated() || project->isCloudProject())
                  ? ""
                  : project->path().toQString());
  setFileModified(project->isNeedSave());
}
} // namespace dgk