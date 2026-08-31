/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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

#include <QColor>
#include <QObject>
#include <QWindow>

namespace dgk
{
/**
 * macOS only: colors the native title bar. MuseScore's MacOSPlatformTheme
 * already makes the title bar transparent, but fills the NSWindow behind it
 * with the Qt palette's window grey; this repaints it with the given color and
 * switches the window to the dark appearance so the title text stays legible.
 * The styling is re-applied on every visibility change, because fullscreen
 * transitions restyle the NSWindow and would otherwise leave the title bar
 * grey again. In QML, declare it after MainWindowBridge — the bridge's window
 * setter is what triggers MuseScore's platform styling, and ours must land on
 * top of it.
 */
class MacOSWindowChrome : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
  Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
  explicit MacOSWindowChrome(QObject *parent = nullptr);

  QWindow *window() const;
  void setWindow(QWindow *window);

  QColor color() const;
  void setColor(const QColor &color);

signals:
  void windowChanged();
  void colorChanged();

private:
  void apply();

  QWindow *m_window = nullptr;
  QColor m_color;
};
} // namespace dgk
