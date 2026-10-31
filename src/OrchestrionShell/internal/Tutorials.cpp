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
#include "Tutorials.h"
#include "DemoCursor.h"
#include "DemoKeys.h"
#include "MuseScoreShell/OrchestrionActionIds.h"
#include "muse_framework_config.h" // MUSE_MODULE_* macros

#ifdef MUSE_MODULE_TESTFLOW
#include <testflow/itestflow.h>
#include <testflow/itestflowscriptsrepository.h>
#endif

#include <QCoreApplication>
#include <QFile>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

namespace dgk
{
namespace
{
/**
 * Whether the comments the script starts with hold `@testflow test`: a test,
 * not a tutorial (see tools/run_testflow.sh).
 */
bool isTest(const QString &path)
{
  QFile file{path};
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;
  static const QRegularExpression leadingComments(
      R"(^(\s*(/\*.*?\*/|//[^\n]*))*)",
      QRegularExpression::DotMatchesEverythingOption);
  static const QRegularExpression testDirective(R"(@testflow\s+test\b)");
  const QString header =
      leadingComments.match(QString::fromUtf8(file.readAll())).captured();
  return testDirective.match(header).hasMatch();
}

// Played when no score is open
constexpr auto defaultScore = "scores/Frere Jacques.mxl";

#ifdef MUSE_MODULE_TESTFLOW
std::shared_ptr<muse::testflow::ITestflow> testflow()
{
  return muse::modularity::ioc(dgk::iocContext())
      ->resolve<muse::testflow::ITestflow>("OrchestrionShell");
}
#endif
} // namespace

Tutorials::Tutorials(std::shared_ptr<DemoCursor> demoCursor,
                     std::shared_ptr<DemoKeys> demoKeys)
    : m_demoCursor{std::move(demoCursor)}, m_demoKeys{std::move(demoKeys)}
{
}

const std::vector<ITutorials::Tutorial> &Tutorials::tutorials() const
{
  // The installed scripts don't change while the application runs.
  if (!m_tutorials)
  {
    m_tutorials.emplace();
#ifdef MUSE_MODULE_TESTFLOW
    const auto repository =
        muse::modularity::ioc(dgk::iocContext())
            ->resolve<muse::testflow::ITestflowScriptsRepository>(
                "OrchestrionShell");
    if (repository)
      for (const muse::testflow::Script &script : repository->scripts())
        if (script.type == muse::testflow::ScriptType::TestCase &&
            !isTest(script.path.toQString()))
          m_tutorials->push_back(
              {script.path.toQString(), script.title, script.description});
#endif
  }
  return *m_tutorials;
}

void Tutorials::run(const QString &path)
{
#ifdef MUSE_MODULE_TESTFLOW
  if (m_running || !testflow())
    return;

  if (!projectSession()->project())
    dispatcher()->dispatch(
        "orchestrion-file-open",
        muse::actions::ActionData::make_arg2<QUrl, QString>(
            QUrl::fromLocalFile(
                (globalConfiguration()->appDataPath() + defaultScore)
                    .toQString()),
            QString{}));

  setRunning(true);
  qApp->installEventFilter(this);
  // Not from within whatever asked for it (a click on the shelf): the
  // script spins nested event loops until it is done.
  QTimer::singleShot(0, this,
                     [this, path]
                     {
                       testflow()->execScript(muse::io::path_t{path});
                       qApp->removeEventFilter(this);
                       cleanUp();
                       setRunning(false);
                     });
#else
  (void)path;
#endif
}

void Tutorials::stop()
{
#ifdef MUSE_MODULE_TESTFLOW
  if (!m_running)
    return;
  // Takes effect once the script is back from what it waits for: put away
  // what it shows right away, which also ends a wait for the player.
  testflow()->abort();
  cleanUp();
#endif
}

bool Tutorials::eventFilter(QObject *watched, QEvent *event)
{
  if (event->type() == QEvent::KeyPress &&
      static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape)
  {
    stop();
    return true;
  }
  return QObject::eventFilter(watched, event);
}

void Tutorials::cleanUp()
{
  if (const auto player = orchestrion()->player();
      player && player->IsPlaying())
    dispatcher()->dispatch(actionIds::playbackStop);
  m_demoKeys->reset();
  m_demoCursor->hide();
}

bool Tutorials::running() const { return m_running; }

muse::async::Notification Tutorials::runningChanged() const
{
  return m_runningChanged;
}

void Tutorials::setRunning(bool running)
{
  if (running == m_running)
    return;
  m_running = running;
  m_runningChanged.notify();
  emit runningStateChanged();
}
} // namespace dgk
