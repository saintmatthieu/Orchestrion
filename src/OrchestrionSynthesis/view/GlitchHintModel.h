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
#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>

#include <QObject>

namespace dgk
{
/**
 * The "Audio glitch?" toast (see IOutputLatency::glitchHintShown): whether
 * it shows, and its link to the latency dialog.
 */
class GlitchHintModel : public QObject,
                        public dgk::Injectable,
                        public muse::async::Asyncable
{
  Q_OBJECT
  Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)

  dgk::Inject<IOutputLatency> outputLatency{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};

public:
  explicit GlitchHintModel(QObject *parent = nullptr);

  bool shown() const;

  /**
   * Opens the latency dialog (as the Audio/MIDI menu does: brought forward
   * if open); the toast goes (till the next glitch).
   */
  Q_INVOKABLE void openSettings();
  Q_INVOKABLE void close();
  Q_INVOKABLE void closeForever();

signals:
  void shownChanged();
};
} // namespace dgk
