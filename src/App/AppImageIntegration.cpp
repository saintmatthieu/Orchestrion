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
#include "AppImageIntegration.h"

#include <log.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

namespace dgk
{
namespace
{
const QString desktopFileName = "orchestrion.desktop";
const QString scheme = "x-scheme-handler/orchestrion";

QByteArray readFile(const QString &path)
{
  QFile file{path};
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}

/** Writes it unless it is there already; whether it was written. */
bool writeIfChanged(const QString &path, const QByteArray &data)
{
  if (readFile(path) == data)
    return false;
  QDir{}.mkpath(QFileInfo{path}.absolutePath());
  QSaveFile file{path};
  if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() ||
      !file.commit())
  {
    LOGW() << "Could not write " << path << ": " << file.errorString();
    return false;
  }
  return true;
}

/** An argument of a desktop entry's Exec key, quoted. */
QString quoted(QString arg)
{
  for (const char c : {'\\', '"', '`', '$'})
    arg.replace(QChar{c}, QString{'\\'} + QChar{c});
  return '"' + arg + '"';
}

void run(const QString &program, const QStringList &arguments)
{
  if (QStandardPaths::findExecutable(program).isEmpty())
    return;
  QProcess::startDetached(program, arguments);
}
} // namespace

void integrateAppImage()
{
#ifdef Q_OS_LINUX
  const QString appImage = qEnvironmentVariable("APPIMAGE");
  const QString appDir = qEnvironmentVariable("APPDIR");
  if (appImage.isEmpty() || appDir.isEmpty())
    return;

  // The desktop entry and icon the AppImage was made with, at its root.
  const QByteArray bundled = readFile(appDir + '/' + desktopFileName);
  if (bundled.isEmpty())
  {
    LOGW() << "No desktop entry in the AppImage";
    return;
  }
  QStringList lines = QString::fromUtf8(bundled).split('\n');
  for (QString &line : lines)
    if (line.startsWith("Exec="))
      line = "Exec=" + quoted(appImage) + " %u";

  const QString dataDir =
      QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
  writeIfChanged(dataDir + "/icons/hicolor/512x512/apps/orchestrion.png",
                 readFile(appDir + "/orchestrion.png"));
  const QString applications = dataDir + "/applications";
  if (!writeIfChanged(applications + '/' + desktopFileName,
                      lines.join('\n').toUtf8()))
    return;
  run("update-desktop-database", {applications});
  // The links are Orchestrion's own: no other application handles them.
  run("xdg-mime", {"default", desktopFileName, scheme});
#endif
}
} // namespace dgk
