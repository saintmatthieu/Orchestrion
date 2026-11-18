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

#include "IOutputLatency.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include <async/asyncable.h>

#include <QObject>
#include <QVariantMap>

namespace dgk
{
/**
 * The latency dialog's model (Audio/MIDI > Latency…): IOutputLatency's
 * status, as a map refreshed several times a second (see
 * IOutputLatency::Status for its fields, named alike), and its controls.
 */
class OutputLatencyModel : public QObject,
                           public dgk::Injectable,
                           public muse::async::Asyncable
{
  Q_OBJECT
  Q_PROPERTY(QVariantMap status READ status NOTIFY statusChanged)

  dgk::Inject<IOutputLatency> outputLatency{this};

public:
  explicit OutputLatencyModel(QObject *parent = nullptr);

  QVariantMap status() const;

  Q_INVOKABLE void setAutomatic(bool automatic);
  Q_INVOKABLE void setLevel(int level);
  Q_INVOKABLE void resetCounters();
  Q_INVOKABLE void delayNextCallback(double ms);

signals:
  void statusChanged();
};
} // namespace dgk
