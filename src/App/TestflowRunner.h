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

#include "CommandOptions.h"

#include <async/asyncable.h>
#include <modularity/ioc.h>
#include <testflow/itestflowconfiguration.h>

namespace dgk
{
/**
 * Runs the testflow script given with --test-case once the main window is
 * up, then ends the process with its result: 0 if it ran a test case to the
 * end, 1 if it failed, 2 if it couldn't be run. Adapted from Audacity's.
 */
class TestflowRunner : public muse::async::Asyncable
{
  muse::GlobalInject<muse::testflow::ITestflowConfiguration> configuration;

public:
  static void runIfRequested(const muse::modularity::ContextPtr &ctx,
                             const CommandOptions::Testflow &options);

private:
  TestflowRunner(const muse::modularity::ContextPtr &ctx,
                 const CommandOptions::Testflow &options,
                 const muse::io::path_t &scriptPath);

  void run();
  void execAndReport();

  static muse::io::path_t resolveScriptPath(const QString &nameOrFile);

  const muse::modularity::ContextPtr m_ctx;
  const CommandOptions::Testflow m_options;
  const muse::io::path_t m_scriptPath;
  int m_startedSteps = 0;
};
} // namespace dgk
