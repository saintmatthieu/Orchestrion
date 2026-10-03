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
#include "StaffModes.h"

#include <engraving/dom/masterscore.h>
#include <engraving/editing/editscoreproperties.h>
#include <notation/imasternotation.h>
#include <notation/inotation.h>
#include <notation/inotationundostack.h>

#include <optional>
#include <sstream>
#include <string>

namespace dgk
{
namespace
{
/**
 * The meta tag, and in it the modes, staff by staff, comma-separated:
 * "rhythm,follow". Written in words so that a score saved by a later version
 * with more modes still reads in this one, the modes it doesn't know as
 * Rhythm.
 */
const muse::String tagName = u"orchestrion-staff-modes";

const char *Name(StaffMode mode)
{
  switch (mode)
  {
  case StaffMode::rhythm:
    return "rhythm";
  case StaffMode::follow:
    return "follow";
  case StaffMode::mute:
    return "mute";
  }
  return "rhythm";
}

std::optional<StaffMode> Parse(const std::string &name)
{
  for (const auto mode :
       {StaffMode::rhythm, StaffMode::follow, StaffMode::mute})
    if (name == Name(mode))
      return mode;
  return std::nullopt;
}
} // namespace

void StaffModes::init()
{
  const auto watchNotation = [this]
  {
    if (const auto master = globalContext()->currentMasterNotation())
      // Edits, undos and redos of the modes all go through the undo stack.
      master->notation()->undoStack()->stackChanged().onNotify(
          this, [this] { checkChanged(); }, Mode::SetReplace);
    checkChanged();
  };
  globalContext()->currentMasterNotationChanged().onNotify(this, watchNotation);
  watchNotation();
}

std::vector<StaffMode> StaffModes::savedModes() const
{
  const auto master = globalContext()->currentMasterNotation();
  if (!master || !master->masterScore())
    return {};
  std::vector<StaffMode> modes;
  std::istringstream names{
      master->masterScore()->metaTag(tagName).toStdString()};
  for (std::string name; std::getline(names, name, ',');)
    modes.push_back(Parse(name).value_or(StaffMode::rhythm));
  return modes;
}

StaffMode StaffModes::mode(int staff) const
{
  const auto modes = savedModes();
  if (staff < 0 || staff >= static_cast<int>(modes.size()))
    return StaffMode::rhythm;
  return modes[staff];
}

void StaffModes::setMode(int staff, StaffMode mode)
{
  const auto master = globalContext()->currentMasterNotation();
  if (!master || !master->masterScore() || staff < 0)
    return;
  auto modes = savedModes();
  if (staff >= static_cast<int>(modes.size()))
    modes.resize(staff + 1, StaffMode::rhythm);
  if (modes[staff] == mode)
    return;
  modes[staff] = mode;
  // Rhythm being what a staff the tag says nothing of is played in, a score
  // left with all its staves in it is left without the tag.
  while (!modes.empty() && modes.back() == StaffMode::rhythm)
    modes.pop_back();
  std::string value;
  for (const auto m : modes)
    value += (value.empty() ? "" : ",") + std::string{Name(m)};

  mu::engraving::MasterScore *const score = master->masterScore();
  std::map<muse::String, muse::String> tags = score->metaTags();
  if (value.empty())
    tags.erase(tagName);
  else
    tags[tagName] = muse::String::fromStdString(value);
  const auto undoStack = master->notation()->undoStack();
  undoStack->prepareChanges(
      muse::TranslatableString("undoableAction", "Change staff mode"));
  score->undo(new mu::engraving::ChangeMetaTags(score, tags));
  undoStack->commitChanges();
}

std::vector<StaffMode> StaffModes::availableModes() const
{
  return {StaffMode::rhythm, StaffMode::follow, StaffMode::mute};
}

muse::async::Notification StaffModes::modesChanged() const
{
  return m_modesChanged;
}

void StaffModes::checkChanged()
{
  auto modes = savedModes();
  while (!modes.empty() && modes.back() == StaffMode::rhythm)
    modes.pop_back();
  if (modes == m_modes)
    return;
  m_modes = std::move(modes);
  m_modesChanged.notify();
}
} // namespace dgk
