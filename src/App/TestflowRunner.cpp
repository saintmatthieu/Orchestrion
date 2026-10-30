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
#include "TestflowRunner.h"

#include <testflow/itestflow.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <cstdlib>
#include <iostream>

#include "log.h"

namespace dgk
{
namespace
{
/**
 * No signal says the UI is ready: the main window loads, then opens the
 * startup score, after the startup scenario has run.
 */
constexpr int settleMs = 3000;

/**
 * Skips the teardown, which isn't what is tested, and which on the null audio
 * device of headless runs aborts. Flush the output first.
 */
[[noreturn]] void exitWithoutTeardown(int code) { std::_Exit(code); }
} // namespace

void TestflowRunner::runIfRequested(const muse::modularity::ContextPtr &ctx,
                                    const CommandOptions::Testflow &options)
{
  if (options.testCaseNameOrFile.isEmpty())
    return;

  // Lives until the process ends, which it does itself.
  (new TestflowRunner(ctx, options,
                      resolveScriptPath(options.testCaseNameOrFile)))
      ->run();
}

TestflowRunner::TestflowRunner(const muse::modularity::ContextPtr &ctx,
                               const CommandOptions::Testflow &options,
                               const muse::io::path_t &scriptPath)
    : m_ctx{ctx}, m_options{options}, m_scriptPath{scriptPath}
{
}

muse::io::path_t TestflowRunner::resolveScriptPath(const QString &nameOrFile)
{
  if (QFileInfo::exists(nameOrFile))
    return muse::io::path_t(QFileInfo(nameOrFile).absoluteFilePath());

  const auto configuration =
      muse::modularity::globalIoc()
          ->resolve<muse::testflow::ITestflowConfiguration>("app");
  IF_ASSERT_FAILED(configuration) { exitWithoutTeardown(2); }

  for (const muse::io::path_t &dir : configuration->scriptsDirPaths())
    for (const QString &candidate : {nameOrFile, nameOrFile + ".js"})
    {
      const QString path = QDir::cleanPath(dir.toQString() + "/" + candidate);
      if (QFileInfo::exists(path))
        return muse::io::path_t(path);
    }

  std::cout << "[testflow] FAILED: script not found: "
            << nameOrFile.toStdString() << std::endl;
  exitWithoutTeardown(2);
}

void TestflowRunner::run()
{
  std::cout << "[testflow] running script=" << m_scriptPath.toStdString()
            << std::endl;
  QTimer::singleShot(settleMs, qApp, [this] { execAndReport(); });
}

void TestflowRunner::execAndReport()
{
  const auto testflow =
      muse::modularity::ioc(m_ctx)->resolve<muse::testflow::ITestflow>("app");
  IF_ASSERT_FAILED(testflow) { exitWithoutTeardown(2); }

  if (!m_options.testCaseSpeed.isEmpty())
    testflow->setSpeedMode(
        muse::testflow::speedModeFromString(m_options.testCaseSpeed));

  muse::testflow::ITestflow::Options opt;
  opt.context = muse::io::path_t(m_options.testCaseContextNameOrFile);
  opt.contextVal = m_options.testCaseContextValue.toStdString();
  opt.func = m_options.testCaseFunc.toStdString();
  opt.funcArgs = m_options.testCaseFuncArgs.toStdString();

  // A script that fails to load, or never runs a test case, still ends up
  // Finished: count the steps to tell these apart.
  testflow->stepStatusChanged().onReceive(
      this,
      [this](const muse::testflow::StepInfo &step, const muse::Ret &)
      {
        if (step.status == muse::testflow::StepStatus::Started)
          ++m_startedSteps;
        // With the wall-clock time, for tools/run_testflow.sh to cut the
        // video it records at a step (see its record-after directive).
        if (step.status == muse::testflow::StepStatus::Started ||
            step.status == muse::testflow::StepStatus::Finished)
          std::cout << "[testflow] step "
                    << (step.status == muse::testflow::StepStatus::Started
                            ? "started"
                            : "finished")
                    << " at=" << QDateTime::currentMSecsSinceEpoch() << " "
                    << step.name.toStdString() << std::endl;
      });

  // Returns once the script has run (its sleeps spin nested event loops).
  testflow->execScript(m_scriptPath, opt);

  const muse::testflow::ITestflow::Status status = testflow->status();
  const bool ok = status == muse::testflow::ITestflow::Status::Finished &&
                  m_startedSteps > 0;

  std::cout << "[testflow] " << (ok ? "PASSED" : "FAILED") << " status="
            << muse::testflow::ITestflow::statusToString(status).toStdString()
            << " steps=" << m_startedSteps
            << " script=" << m_scriptPath.toStdString()
            << " reports=" << configuration()->reportsPath().toStdString()
            << std::endl;
  if (m_startedSteps == 0)
    std::cout << "[testflow] no steps were executed - does the script define "
                 "main() and run a test case?"
              << std::endl;

  exitWithoutTeardown(ok ? 0 : 1);
}
} // namespace dgk
