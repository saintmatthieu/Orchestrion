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
#include <QUrl>
#include <functional>
#include <memory>
#include <vector>

class QLocalServer;
class QLocalSocket;

namespace dgk
{
/**
 * Makes Orchestrion a single instance, and receives what it is asked to open
 * from outside while it runs.
 *
 * On Windows and Linux, every launch is a new process: one that finds
 * Orchestrion running hands it the score or link it was given, if any, and
 * quits (forwardToRunningInstance()). The running one receives it through a
 * local socket of the user's. On macOS, the system sends the files and links
 * to the running application as QFileOpenEvent, the first ones possibly
 * before it is set up.
 *
 * What comes before there is a handler is kept for it.
 */
class OpenRequestReceiver : public QObject
{
public:
  /**
   * Whether an instance already running took the URL (empty: none, just
   * come to the front): this one then has nothing to do.
   */
  static bool forwardToRunningInstance(const QUrl &url);

  OpenRequestReceiver();
  ~OpenRequestReceiver() override;

  void setHandler(std::function<void(const QUrl &)> handler);

private:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void onNewConnection();
  void onReadyRead(QLocalSocket *socket);
  void receive(const QUrl &url);

  std::unique_ptr<QLocalServer> m_server;
  std::function<void(const QUrl &)> m_handler;
  std::vector<QUrl> m_received;
};
} // namespace dgk
