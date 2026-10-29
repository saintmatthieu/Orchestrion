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
#include <muse_framework_config.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileOpenEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <utility>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace dgk
{
namespace
{
/** A request is a URL, encoded, on a line of its own. */
constexpr qint64 maxRequestLength = 8192;
constexpr int timeoutMs = 1000;

/**
 * The user's (a named pipe on Windows is seen by all users), and the
 * application's: a development build doesn't take a release's requests, nor
 * those of another build tree's.
 */
QString serverName()
{
  QString owner = QDir::homePath();
#ifdef MUSE_APP_UNSTABLE
  owner += QCoreApplication::applicationDirPath();
#endif
  const QByteArray hash =
      QCryptographicHash::hash(owner.toUtf8(), QCryptographicHash::Sha1)
          .toHex()
          .left(16);
  return QCoreApplication::applicationName() + '-' + QString::fromLatin1(hash);
}
} // namespace

bool OpenRequestReceiver::forwardToRunningInstance(const QUrl &url)
{
#ifdef Q_OS_MACOS
  Q_UNUSED(url);
  return false;
#else
  QLocalSocket socket;
  socket.connectToServer(serverName());
  if (!socket.waitForConnected(timeoutMs))
    return false;
#ifdef Q_OS_WIN
  // Windows lets only the foreground process bring a window to the front:
  // this one, launched by the click.
  AllowSetForegroundWindow(ASFW_ANY);
#endif
  socket.write(url.toEncoded() + '\n');
  if (!socket.waitForBytesWritten(timeoutMs))
  {
    LOGW() << "Orchestrion is running but doesn't answer";
    return false;
  }
  socket.disconnectFromServer();
  if (socket.state() != QLocalSocket::UnconnectedState)
    socket.waitForDisconnected(timeoutMs);
  return true;
#endif
}

OpenRequestReceiver::OpenRequestReceiver()
{
#ifdef Q_OS_MACOS
  qApp->installEventFilter(this);
#else
  m_server = std::make_unique<QLocalServer>();
  m_server->setSocketOptions(QLocalServer::UserAccessOption);
  const QString name = serverName();
  if (!m_server->listen(name))
  {
    // What a crashed instance left (a socket file, on Linux): nothing
    // answered it.
    QLocalServer::removeServer(name);
    if (!m_server->listen(name))
      LOGW() << "Can't receive the requests of other launches: "
             << m_server->errorString();
  }
  connect(m_server.get(), &QLocalServer::newConnection, this,
          &OpenRequestReceiver::onNewConnection);
#endif
}

OpenRequestReceiver::~OpenRequestReceiver()
{
#ifdef Q_OS_MACOS
  if (qApp)
    qApp->removeEventFilter(this);
#endif
}

void OpenRequestReceiver::setHandler(std::function<void(const QUrl &)> handler)
{
  m_handler = std::move(handler);
  for (const QUrl &url : std::exchange(m_received, {}))
    m_handler(url);
}

bool OpenRequestReceiver::eventFilter(QObject *watched, QEvent *event)
{
  if (watched == qApp && event->type() == QEvent::FileOpen)
  {
    receive(static_cast<QFileOpenEvent *>(event)->url());
    return true;
  }
  return QObject::eventFilter(watched, event);
}

void OpenRequestReceiver::onNewConnection()
{
  while (QLocalSocket *const socket = m_server->nextPendingConnection())
  {
    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    connect(socket, &QLocalSocket::readyRead, this,
            [this, socket] { onReadyRead(socket); });
    // It may all be there already.
    onReadyRead(socket);
  }
}

void OpenRequestReceiver::onReadyRead(QLocalSocket *socket)
{
  if (!socket->canReadLine())
  {
    if (socket->bytesAvailable() > maxRequestLength)
      socket->abort();
    return;
  }
  const QByteArray line = socket->readLine(maxRequestLength).trimmed();
  socket->disconnectFromServer();
  receive(QUrl::fromEncoded(line, QUrl::StrictMode));
}

void OpenRequestReceiver::receive(const QUrl &url)
{
  if (m_handler)
    m_handler(url);
  else
    m_received.push_back(url);
}
} // namespace dgk
