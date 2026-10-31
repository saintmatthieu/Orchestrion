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

#include "ITutorials.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include "OrchestrionSequencer/IOrchestrion.h"

#include <actions/iactionsdispatcher.h>
#include <global/iglobalconfiguration.h>
#include <project/iprojectsession.h>

#include <QObject>
#include <memory>
#include <optional>

namespace dgk
{
class DemoCursor;
class DemoKeys;

class Tutorials : public QObject, public ITutorials, public dgk::Injectable
{
  Q_OBJECT

  /** For the overlay's "Esc to stop" (DemoKeysOverlay.qml) */
  Q_PROPERTY(bool running READ running NOTIFY runningStateChanged)

  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};
  dgk::Inject<mu::project::IProjectSession> projectSession{this};
  dgk::Inject<IOrchestrion> orchestrion{this};

public:
  Tutorials(std::shared_ptr<DemoCursor> demoCursor,
            std::shared_ptr<DemoKeys> demoKeys);

  const std::vector<Tutorial> &tutorials() const override;
  void run(const QString &path) override;
  void stop() override;
  bool running() const override;
  muse::async::Notification runningChanged() const override;

signals:
  void runningStateChanged();

private:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void setRunning(bool running);
  /** Puts away what the tutorial showed, and stops the player. */
  void cleanUp();

  const std::shared_ptr<DemoCursor> m_demoCursor;
  const std::shared_ptr<DemoKeys> m_demoKeys;
  mutable std::optional<std::vector<Tutorial>> m_tutorials;
  bool m_running = false;
  muse::async::Notification m_runningChanged;
};
} // namespace dgk
