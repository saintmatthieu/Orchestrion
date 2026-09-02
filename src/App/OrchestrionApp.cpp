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
#include "OrchestrionApp.h"

#include <engraving/dom/mscore.h>
#include <engraving/types/constants.h>
#include <log.h>
#include <project/types/migrationtypes.h>

namespace dgk
{
OrchestrionApp::OrchestrionApp(const std::shared_ptr<CommandOptions> &options)
    : muse::ui::GuiApplication(options)
{
}

void OrchestrionApp::applyCommandLineOptions(
    const std::shared_ptr<muse::CmdOptions> &opt)
{
  muse::ui::GuiApplication::applyCommandLineOptions(opt);

  const std::shared_ptr<CommandOptions> options =
      std::dynamic_pointer_cast<CommandOptions>(opt);
  IF_ASSERT_FAILED(options) { return; }

  if (options->startup.scoreUrl.has_value())
  {
    StartupProjectFile file{
        *options->startup.scoreUrl,
        options->startup.scoreDisplayNameOverride.value_or("")};
    startupScenario()->setStartupScoreFile(file);
  }

  // Migration options: keep everything false by default: we don't want to
  // modify a user's score, and we don't want conversion to happen on every
  // load.
  mu::project::MigrationOptions migration;
  migration.appVersion = mu::engraving::Constants::MSC_VERSION;
  migration.isAskAgain = false;
  migration.isApplyMigration = false;
  migration.isApplyEdwin = false;
  migration.isApplyLeland = false;
  migration.isRemapPercussion = false;
  if (options->project.fullMigration)
  {
    const bool isMigration = options->project.fullMigration.value();
    migration.isApplyMigration = isMigration;
    migration.isApplyEdwin = isMigration;
    migration.isApplyLeland = isMigration;
    migration.isRemapPercussion = isMigration;
  }
  //! NOTE Don't write to settings, just on current session
  for (mu::project::MigrationType type : mu::project::allMigrationTypes())
    projectConfiguration()->setMigrationOptions(type, migration, false);
}

QString OrchestrionApp::mainWindowQmlPath(const QString &) const
{
  return QStringLiteral("qrc:/qt/qml/Orchestrion/src/qml/Main.qml");
}

void OrchestrionApp::doStartupScenario(const muse::modularity::ContextPtr &)
{
  // Orchestrion's main window drives its own startup (see
  // OrchestrionOnboardingModel, which opens the startup score).
}
} // namespace dgk
