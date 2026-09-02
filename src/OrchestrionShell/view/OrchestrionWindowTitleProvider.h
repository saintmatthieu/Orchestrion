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
#pragma once

#include "OrchestrionSequencer/IOrchestrion.h"

#include "async/asyncable.h"
#include "context/iglobalcontext.h"

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
class OrchestrionWindowTitleProvider : public QObject,
                                       public dgk::Injectable,
                                       public muse::async::Asyncable
{
  Q_OBJECT

  dgk::Inject<mu::context::IGlobalContext> context{this};
  dgk::Inject<IOrchestrion> orchestrion{this};

  Q_PROPERTY(QString title READ title NOTIFY titleChanged)
  //! The work's title as displayed on the score itself (not the window):
  //! the score's "workTitle" meta tag, or the file's base name when there is
  //! none.
  Q_PROPERTY(QString scoreTitle READ scoreTitle NOTIFY scoreTitleChanged)
  //! The score's "composer" meta tag, shown under the title; may be empty.
  Q_PROPERTY(
      QString scoreComposer READ scoreComposer NOTIFY scoreComposerChanged)
  Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
  Q_PROPERTY(bool fileModified READ fileModified NOTIFY fileModifiedChanged)

public:
  explicit OrchestrionWindowTitleProvider(QObject *parent = nullptr);

  Q_INVOKABLE void load();

  QString title() const;
  QString scoreTitle() const;
  QString scoreComposer() const;
  QString filePath() const;
  bool fileModified() const;

signals:
  void titleChanged(QString title);
  void scoreTitleChanged(QString scoreTitle);
  void scoreComposerChanged(QString scoreComposer);
  void filePathChanged(QString filePath);
  void fileModifiedChanged(bool fileModified);

private:
  void update();

  void setTitle(const QString &title);
  void setScoreTitle(const QString &scoreTitle);
  void setScoreComposer(const QString &scoreComposer);
  void setFilePath(const QString &filePath);
  void setFileModified(bool fileModified);

  QString m_title;
  QString m_scoreTitle;
  QString m_scoreComposer;
  QString m_filePath;
  bool m_fileModified;
};
} // namespace dgk
