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

#include <async/channel.h>
#include <async/notification.h>
#include <modularity/imoduleinterface.h>
#include <optional>

namespace dgk
{
class IComputerKeyboard : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IComputerKeyboard);

public:
  enum class Layout
  {
    German,
    US,
  };

  static std::string layoutToString(Layout layout)
  {
    switch (layout)
    {
    case Layout::German:
      return "Deutsch";
    case Layout::US:
      return "US";
    }
    return "";
  }

  static std::optional<Layout> layoutFromString(const std::string &layout)
  {
    if (layout == "Deutsch")
      return Layout::German;
    if (layout == "US")
      return Layout::US;
    return {};
  }

  static std::vector<std::string> availableLayouts()
  {
    std::vector<std::string> layouts;
    for (auto layout : {Layout::German, Layout::US})
      layouts.push_back(layoutToString(layout));
    return layouts;
  }

  virtual Layout layout() const = 0;
  virtual void setLayout(Layout) = 0;
  virtual muse::async::Notification layoutChanged() const = 0;
  virtual muse::async::Channel<char> keyPressed() const = 0;
  virtual muse::async::Channel<char> keyReleased() const = 0;
};
} // namespace dgk