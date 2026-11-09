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

#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

class QLocalServer;

namespace dgk
{
/**
 * Receives what is asked of the running application from outside: on Windows
 * and Linux from a second process, started with a link or a file, which hands
 * it over and quits; on macOS from the system, as a QFileOpenEvent.
 *
 * Made right after the QApplication, and not by a module, because macOS sends
 * the event of a link that launched the application as soon as the event loop
 * runs, which is before the modules are set up. What comes before
 * setHandler() is kept for it.
 */
class OpenRequestReceiver : public QObject
{
public:
  OpenRequestReceiver();
  ~OpenRequestReceiver() override;

  /**
   * Hands the request to an instance already running.
   * @return whether one took it.
   */
  bool forwardToRunningInstance(const QString &request) const;

  /** Becomes the instance others hand requests to, if none is. */
  void listen();

  void setHandler(std::function<void(const QString &)> handler);

private:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void onNewConnection();
  void receive(const QString &request);

  QLocalServer *m_server = nullptr;
  std::function<void(const QString &)> m_handler;
  QStringList m_received;
};
} // namespace dgk
