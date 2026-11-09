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
#include "OpenRequestReceiver.h"

#include <log.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileOpenEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QUrl>
#include <utility>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace dgk
{
namespace
{
constexpr qint64 maxRequestBytes = 64 * 1024;
constexpr int probeTimeoutMs = 200;
/**
 * The running instance answers from its main thread, which may be busy for
 * a moment. Past this it is taken for hung, and this process carries on.
 */
constexpr int answerTimeoutMs = 5000;

/** Per user, and per application name, as the settings are. */
QString serverName()
{
#ifdef Q_OS_WIN
  const QString user = qEnvironmentVariable("USERNAME");
#else
  const QString user = qEnvironmentVariable("USER");
#endif
  const QString hash = QString::fromLatin1(
      QCryptographicHash::hash(user.toUtf8(), QCryptographicHash::Sha256)
          .toHex()
          .left(16));
  const QString name = QCoreApplication::applicationName() + '-' + hash;
#ifdef Q_OS_WIN
  return name;
#else
  // Rather than the shared temporary directory.
  return QDir{QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)}
      .filePath(name);
#endif
}
} // namespace

OpenRequestReceiver::OpenRequestReceiver()
{
  QCoreApplication::instance()->installEventFilter(this);
}

OpenRequestReceiver::~OpenRequestReceiver() = default;

bool OpenRequestReceiver::forwardToRunningInstance(const QString &request) const
{
#ifdef Q_OS_MACOS
  // The system never starts a second process for these.
  Q_UNUSED(request);
  return false;
#else
  QLocalSocket socket;
  socket.connectToServer(serverName());
  if (!socket.waitForConnected(probeTimeoutMs))
    return false;
#ifdef Q_OS_WIN
  // This process was started by the user's click, so it may give the other
  // one the right to come to the front.
  AllowSetForegroundWindow(ASFW_ANY);
#endif
  socket.write(request.toUtf8() + '\n');
  if (!socket.waitForBytesWritten(answerTimeoutMs))
    return false;
  // Waited for: a process quitting too soon could take the request with it.
  while (!socket.canReadLine())
    if (!socket.waitForReadyRead(answerTimeoutMs))
      return false;
  return true;
#endif
}

void OpenRequestReceiver::listen()
{
#ifndef Q_OS_MACOS
  const QString name = serverName();
  QLocalSocket probe;
  probe.connectToServer(name);
  if (probe.waitForConnected(probeTimeoutMs))
    return;
  // What a crashed instance left behind.
  QLocalServer::removeServer(name);
  m_server = new QLocalServer{this};
  m_server->setSocketOptions(QLocalServer::UserAccessOption);
  connect(m_server, &QLocalServer::newConnection, this,
          &OpenRequestReceiver::onNewConnection);
  if (!m_server->listen(name))
    LOGW() << "Not receiving links from other processes: "
           << m_server->errorString();
#endif
}

void OpenRequestReceiver::onNewConnection()
{
  while (QLocalSocket *const socket = m_server->nextPendingConnection())
  {
    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    connect(
        socket, &QLocalSocket::readyRead, this,
        [this, socket]
        {
          if (!socket->canReadLine())
          {
            if (socket->bytesAvailable() > maxRequestBytes)
              socket->abort();
            return;
          }
          const QString request =
              QString::fromUtf8(socket->readLine(maxRequestBytes)).trimmed();
          socket->write("ok\n");
          socket->flush();
          receive(request);
        });
  }
}

bool OpenRequestReceiver::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() != QEvent::FileOpen)
    return QObject::eventFilter(watched, event);
  const QUrl url = static_cast<QFileOpenEvent *>(event)->url();
  receive(url.isLocalFile() ? url.toLocalFile() : url.toString());
  return true;
}

void OpenRequestReceiver::receive(const QString &request)
{
  if (request.isEmpty())
    return;
  if (m_handler)
    m_handler(request);
  else
    m_received << request;
}

void OpenRequestReceiver::setHandler(
    std::function<void(const QString &)> handler)
{
  m_handler = std::move(handler);
  for (const QString &request : std::exchange(m_received, {}))
    m_handler(request);
}
} // namespace dgk
