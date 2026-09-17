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

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "ui/iuiconfiguration.h"
#include <QColor>
#include <QObject>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace dgk
{
/**
 * The Orchestrion palette, as a QML singleton — see OrchestrionPalette.h for
 * where the colours come from and why they live in MuseScore's theme files.
 *
 * QML reaches it through the `Theme` singleton, which forwards to this one:
 * `ui` is a root context property rather than a singleton, so a pure-QML
 * palette could not read the theme itself.
 *
 * Every property re-reads the current theme, and they all share one change
 * signal, so a theme switch re-evaluates every binding at once.
 */
class OrchestrionTheme : public QObject,
                         public dgk::Injectable,
                         public muse::async::Asyncable
{
  Q_OBJECT

  //! "gold" or "silver".
  Q_PROPERTY(QString name READ name NOTIFY paletteChanged)

  Q_PROPERTY(QColor backdrop READ backdrop NOTIFY paletteChanged)
  Q_PROPERTY(QColor accent READ accent NOTIFY paletteChanged)
  Q_PROPERTY(QColor accentInk READ accentInk NOTIFY paletteChanged)
  Q_PROPERTY(QColor metal READ metal NOTIFY paletteChanged)
  Q_PROPERTY(QColor metalBright READ metalBright NOTIFY paletteChanged)
  Q_PROPERTY(QColor inkMuted READ inkMuted NOTIFY paletteChanged)
  Q_PROPERTY(QColor inkFaint READ inkFaint NOTIFY paletteChanged)
  Q_PROPERTY(QColor highlight READ highlight NOTIFY paletteChanged)
  Q_PROPERTY(QColor overlay READ overlay NOTIFY paletteChanged)
  Q_PROPERTY(QColor popup READ popup NOTIFY paletteChanged)
  Q_PROPERTY(QColor toast READ toast NOTIFY paletteChanged)

  dgk::Inject<muse::ui::IUiConfiguration> uiConfiguration{this};

public:
  explicit OrchestrionTheme(QObject *parent = nullptr);

  QString name() const;

  QColor backdrop() const;
  QColor accent() const;
  QColor accentInk() const;
  QColor metal() const;
  QColor metalBright() const;
  QColor inkMuted() const;
  QColor inkFaint() const;
  QColor highlight() const;
  QColor overlay() const;
  QColor popup() const;
  QColor toast() const;

signals:
  void paletteChanged();
};
} // namespace dgk
