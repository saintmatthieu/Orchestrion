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

#include "async/asyncable.h"
#include "context/iglobalcontext.h"

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
/**
 * Exposes the current score's title and composer to QML, for the ornament
 * above the notation. The window title itself is a constant: it deliberately
 * shows neither the file name nor a modified marker.
 */
class OrchestrionWindowTitleProvider : public QObject,
                                       public dgk::Injectable,
                                       public muse::async::Asyncable
{
  Q_OBJECT

  dgk::Inject<mu::context::IGlobalContext> context{this};

  //! The work's title as displayed on the score itself (not the window):
  //! the score's "workTitle" meta tag, or the file's base name when there is
  //! none.
  Q_PROPERTY(QString scoreTitle READ scoreTitle NOTIFY scoreTitleChanged)
  //! The score's "composer" meta tag, shown under the title; may be empty.
  Q_PROPERTY(
      QString scoreComposer READ scoreComposer NOTIFY scoreComposerChanged)

public:
  explicit OrchestrionWindowTitleProvider(QObject *parent = nullptr);

  Q_INVOKABLE void load();

  QString scoreTitle() const;
  QString scoreComposer() const;

signals:
  void scoreTitleChanged(QString scoreTitle);
  void scoreComposerChanged(QString scoreComposer);

private:
  void update();

  void setScoreTitle(const QString &scoreTitle);
  void setScoreComposer(const QString &scoreComposer);

  QString m_scoreTitle;
  QString m_scoreComposer;
};
} // namespace dgk
