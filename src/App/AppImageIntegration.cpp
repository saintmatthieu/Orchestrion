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
const QString desktopFileName = QStringLiteral("orchestrion-appimage.desktop");

/**
 * As the desktop entry specification wants an Exec argument: quoted, for the
 * command line first, and then for the file's string syntax.
 */
QString execArgument(QString path)
{
  for (const char *const special : {"\\", "\"", "`", "$"})
    path.replace(QLatin1String(special), '\\' + QLatin1String(special));
  path.replace('\\', QStringLiteral("\\\\"));
  path.replace('%', QStringLiteral("%%"));
  return '"' + path + '"';
}

QString desktopEntry(const QString &appImage, const QString &icon)
{
  return QStringLiteral("[Desktop Entry]\n"
                        "Version=1.0\n"
                        "Type=Application\n"
                        "Name=Orchestrion\n"
                        "Comment=Music practice and performance\n"
                        "Exec=%1 %u\n"
                        "Icon=%2\n"
                        "Terminal=false\n"
                        "Categories=AudioVideo;Audio;Sequencer;Midi;Qt;\n"
                        "MimeType=x-scheme-handler/orchestrion;"
                        "application/vnd.recordare.musicxml;"
                        "application/vnd.recordare.musicxml+xml;\n")
      .arg(execArgument(appImage), icon);
}

void run(const QString &program, const QStringList &arguments)
{
  if (!QProcess::startDetached(program, arguments))
    LOGI() << "Could not run " << program;
}
} // namespace

void registerAppImageWithDesktop()
{
  const QString appImage = qEnvironmentVariable("APPIMAGE");
  if (appImage.isEmpty() || appImage.contains('\n'))
    return;

  const QDir data{
      QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
  const QString directory = data.filePath(QStringLiteral("applications"));
  const QString icon = data.filePath(QStringLiteral("icons/orchestrion.png"));
  const QString path = QDir{directory}.filePath(desktopFileName);
  const QByteArray entry = desktopEntry(appImage, icon).toUtf8();

  // Unchanged, it is left alone, and so is whichever handler the user chose.
  if (QFile existing{path};
      existing.open(QIODevice::ReadOnly) && existing.readAll() == entry)
    return;

  QDir{}.mkpath(directory);
  QSaveFile file{path};
  if (!file.open(QIODevice::WriteOnly) || file.write(entry) != entry.size() ||
      !file.commit())
  {
    LOGW() << "Could not write " << path << ": " << file.errorString();
    return;
  }

  // The window's own icon is in the AppImage, which the desktop can't reach.
  QDir{}.mkpath(QFileInfo{icon}.absolutePath());
  QFile::remove(icon);
  QFile::copy(qEnvironmentVariable("APPDIR") + "/.DirIcon", icon);

  run(QStringLiteral("update-desktop-database"), {directory});
  run(QStringLiteral("xdg-mime"),
      {QStringLiteral("default"), desktopFileName,
       QStringLiteral("x-scheme-handler/orchestrion")});
}
} // namespace dgk
