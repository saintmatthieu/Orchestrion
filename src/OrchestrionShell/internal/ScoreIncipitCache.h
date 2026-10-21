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

#include <engraving/rendering/iscorerenderer.h>
#include <global/iglobalconfiguration.h>
#include <project/iprojectcreator.h>

#include <QString>
#include <optional>

#include "OrchestrionCommon/OrchestrionIoc.h"

namespace dgk
{
/**
 * What the score shelf shows of a score without opening it: its title and
 * composer, and its incipit — the beginning of its first system, drawn as a
 * strip the shelf's cards are made to hold.
 *
 * Getting these means loading the score, which is far too slow to do for a
 * whole shelf every time it is shown, so they are kept on disk (in the user's
 * app data, keyed by the file's path, size and modification time) and only
 * rendered once per version of a file.
 */
class ScoreIncipitCache : public dgk::Injectable
{
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};
  dgk::Inject<mu::project::IProjectCreator> projectCreator{this};
  dgk::Inject<mu::engraving::rendering::IScoreRenderer> scoreRenderer{this};

public:
  /**
   * The strip's size in logical pixels, and the device pixel ratio it is
   * rendered for.
   */
  static constexpr int width = 320;
  static constexpr int height = 112;
  static constexpr int pixelRatio = 2;

  struct Entry
  {
    QString title;
    QString composer;
    /**
     * The incipit's PNG; empty if the score could not be drawn (it then
     * isn't tried again until the file changes).
     */
    QString imagePath;
  };

  /** The entry for the file as it is now, if it was rendered already. */
  std::optional<Entry> cached(const QString &scorePath) const;

  /**
   * Loads the score and renders its entry, and keeps it for next time.
   * Blocks for as long as loading and laying out the score takes: a fraction
   * of a second for most scores, more for long ones.
   */
  Entry render(const QString &scorePath);

private:
  QString directory() const;
  /** The file name stem shared by an entry's JSON and PNG. */
  QString key(const QString &scorePath) const;
};
} // namespace dgk
