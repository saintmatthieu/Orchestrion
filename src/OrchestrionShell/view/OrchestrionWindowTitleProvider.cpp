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
#include "translation.h"

namespace dgk
{
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

          currentProject->needSave().notification.onNotify(this, [this]()
                                                           { update(); });
        }
      });

  context()->currentNotationChanged().onNotify(this, [this]() { update(); });
}

QString OrchestrionWindowTitleProvider::title() const { return m_title; }

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
  mu::project::INotationProjectPtr project = context()->currentProject();

  if (!project)
  {
    setTitle(muse::qtrc("appshell", "Orchestrion"));
    setFilePath("");
    setFileModified(false);
    return;
  }

  const auto notation = context()->currentNotation();
  setTitle(notation->projectNameAndPartName());

  setFilePath((project->isNewlyCreated() || project->isCloudProject())
                  ? ""
                  : project->path().toQString());
  setFileModified(project->needSave().val);
}
} // namespace dgk