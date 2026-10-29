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
#include "ScoreHeadingModel.h"
#include "OrchestrionShell/internal/ScoreTitles.h"
#include <project/inotationproject.h>
#include <project/types/projectmeta.h>

namespace dgk
{
ScoreHeadingModel::ScoreHeadingModel(QObject *parent) : QObject(parent) {}

void ScoreHeadingModel::load()
{
  update();

  projectSession()->projectChanged().onNotify(
      this,
      [this]()
      {
        update();
        if (auto currentProject = projectSession()->project())
          currentProject->displayNameChanged().onNotify(this,
                                                        [this]() { update(); });
      });
}

QString ScoreHeadingModel::scoreTitle() const { return m_scoreTitle; }

QString ScoreHeadingModel::scoreComposer() const { return m_scoreComposer; }

void ScoreHeadingModel::setScoreTitle(const QString &scoreTitle)
{
  if (scoreTitle == m_scoreTitle)
  {
    return;
  }

  m_scoreTitle = scoreTitle;
  emit scoreTitleChanged(scoreTitle);
}

void ScoreHeadingModel::setScoreComposer(const QString &scoreComposer)
{
  if (scoreComposer == m_scoreComposer)
  {
    return;
  }

  m_scoreComposer = scoreComposer;
  emit scoreComposerChanged(scoreComposer);
}

void ScoreHeadingModel::update()
{
  const mu::project::INotationProjectPtr project = projectSession()->project();
  if (!project)
  {
    setScoreTitle("");
    setScoreComposer("");
    return;
  }

  const mu::project::ProjectMeta meta = project->metaInfo();
  // Qualified: the members of the same names would hide them.
  QString title = dgk::scoreTitle(meta, project->path());
  if (title.isEmpty())
    title = project->displayName();
  setScoreTitle(title);
  setScoreComposer(dgk::scoreComposer(meta));
}
} // namespace dgk