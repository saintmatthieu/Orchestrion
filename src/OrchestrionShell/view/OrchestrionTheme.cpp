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
#include "OrchestrionTheme.h"
#include "OrchestrionCommon/OrchestrionPalette.h"

namespace dgk
{
OrchestrionTheme::OrchestrionTheme(QObject *parent) : QObject(parent)
{
  uiConfiguration()->currentThemeChanged().onNotify(
      this, [this] { emit paletteChanged(); });
}

QString OrchestrionTheme::name() const
{
  return paletteString(uiConfiguration()->currentTheme(),
                       paletteKeys::themeName);
}

QColor OrchestrionTheme::backdrop() const
{
  return paletteColor(uiConfiguration()->currentTheme(),
                      paletteKeys::backdrop);
}

QColor OrchestrionTheme::accent() const
{
  return paletteColor(uiConfiguration()->currentTheme(), paletteKeys::accent);
}

QColor OrchestrionTheme::accentInk() const
{
  return paletteColor(uiConfiguration()->currentTheme(),
                      paletteKeys::accentInk);
}

QColor OrchestrionTheme::metal() const
{
  return paletteColor(uiConfiguration()->currentTheme(), paletteKeys::metal);
}

QColor OrchestrionTheme::metalBright() const
{
  return paletteColor(uiConfiguration()->currentTheme(),
                      paletteKeys::metalBright);
}

QColor OrchestrionTheme::inkMuted() const
{
  return paletteColor(uiConfiguration()->currentTheme(),
                      paletteKeys::inkMuted);
}

QColor OrchestrionTheme::inkFaint() const
{
  return paletteColor(uiConfiguration()->currentTheme(),
                      paletteKeys::inkFaint);
}

QColor OrchestrionTheme::highlight() const
{
  return paletteColor(uiConfiguration()->currentTheme(),
                      paletteKeys::highlight);
}

QColor OrchestrionTheme::overlay() const
{
  return paletteColor(uiConfiguration()->currentTheme(), paletteKeys::overlay);
}

QColor OrchestrionTheme::popup() const
{
  return paletteColor(uiConfiguration()->currentTheme(), paletteKeys::popup);
}

QColor OrchestrionTheme::toast() const
{
  return paletteColor(uiConfiguration()->currentTheme(), paletteKeys::toast);
}
} // namespace dgk
