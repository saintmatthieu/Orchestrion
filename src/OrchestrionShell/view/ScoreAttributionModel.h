/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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

#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "global/iglobalconfiguration.h"
#include "global/iinteractive.h"
#include "modularity/ioc.h"

#include <QHash>
#include <QObject>
#include <QString>

namespace dgk
{
/*!
 * Shows the attribution that a score must be credited with (e.g. the author of
 * a Creative-Commons transcription) when that score is opened.
 *
 * Scores are marked as needing attribution in scores/attributions.json, keyed
 * by file name. When the current project changes, this model looks the opened
 * file up in that manifest and, if found, emits attributionRequired() so the
 * UI can show a transient toast.
 */
class ScoreAttributionModel : public QObject, public muse::async::Asyncable
{
  Q_OBJECT

  muse::Inject<mu::context::IGlobalContext> context;
  muse::Inject<muse::IGlobalConfiguration> globalConfiguration;
  muse::Inject<muse::IInteractive> interactive;

  Q_PROPERTY(QString author READ author NOTIFY attributionChanged)
  Q_PROPERTY(QString license READ license NOTIFY attributionChanged)
  Q_PROPERTY(QString url READ url NOTIFY attributionChanged)

public:
  explicit ScoreAttributionModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();
  Q_INVOKABLE void openSourceUrl() const;

  QString author() const;
  QString license() const;
  QString url() const;

signals:
  void attributionChanged();

  //! Emitted when a score that must be attributed has just been opened.
  void attributionRequired();

private:
  struct Attribution
  {
    QString author;
    QString license;
    QString url;
  };

  void loadManifest();
  void onCurrentProjectChanged();

  QHash<QString, Attribution> m_attributions;
  Attribution m_current;
};
} // namespace dgk
