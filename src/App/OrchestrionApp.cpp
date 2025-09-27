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

#include <global/globalmodule.h>
#include <global/modularity/ioc.h>
#include <ui/iuiengine.h>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>

#include <future>

static muse::GlobalModule globalModule;

namespace dgk
{
OrchestrionApp::OrchestrionApp(CommandOptions options)
    : m_opts{std::move(options)},
      m_context(std::make_shared<muse::modularity::Context>())
{
}

void OrchestrionApp::addModule(muse::modularity::IModuleSetup *module)
{
  m_modules.push_back(module);
};

void OrchestrionApp::perform()
{
  globalModule.setApplication(shared_from_this());
  globalModule.registerResources();
  globalModule.registerExports();
  globalModule.registerUiTypes();

  for (muse::modularity::IModuleSetup *m : m_modules)
  {
    m->setApplication(globalModule.application());
    m->registerResources();
  }

  for (muse::modularity::IModuleSetup *m : m_modules)
    m->registerExports();

  globalModule.resolveImports();
  globalModule.registerApi();
  for (muse::modularity::IModuleSetup *m : m_modules)
  {
    m->registerUiTypes();
    m->resolveImports();
    m->registerApi();
  }

  if (m_opts.startup.scoreUrl.has_value())
  {
    StartupProjectFile file{
        *m_opts.startup.scoreUrl,
        m_opts.startup.scoreDisplayNameOverride.value_or("").toStdString()};

    if (m_opts.startup.scoreDisplayNameOverride.has_value())
      file.displayNameOverride =
          m_opts.startup.scoreDisplayNameOverride->toStdString();

    startupScenario()->setStartupScoreFile(file);
  }

  // set migration options
  {
    mu::project::MigrationOptions migration;
    migration.appVersion = mu::engraving::Constants::MSC_VERSION;

    migration.isAskAgain = false;
    // Keep everything false by default: we don't want to modify a user's score,
    // and we don't want conversion to happen on every load.
    migration.isApplyMigration = false;
    migration.isApplyEdwin = false;
    migration.isApplyLeland = false;
    migration.isRemapPercussion = false;

    if (m_opts.project.fullMigration)
    {
      bool isMigration = m_opts.project.fullMigration.value();
      migration.isApplyMigration = isMigration;
      migration.isApplyEdwin = isMigration;
      migration.isApplyLeland = isMigration;
      migration.isRemapPercussion = isMigration;
    }

    //! NOTE Don't write to settings, just on current session
    for (mu::project::MigrationType type : mu::project::allMigrationTypes())
      projectConfiguration()->setMigrationOptions(type, migration, false);
  }

  constexpr auto runMode = muse::IApplication::RunMode::GuiApp;
  globalModule.onPreInit(runMode);
  for (muse::modularity::IModuleSetup *m : m_modules)
    m->onPreInit(runMode);

  globalModule.onInit(runMode);
  for (muse::modularity::IModuleSetup *m : m_modules)
  {
    if (m->moduleName() == "audio_engine")
    {
      // We have to spawn a thread to init this one and use a future to wait for
      // it.
      std::promise<void> promise;
      auto future = promise.get_future();
      std::thread(
          [&]
          {
            m->onInit(runMode);
            promise.set_value();
          })
          .detach();
      future.wait();
    }
    else
      m->onInit(runMode);
  }

  globalModule.onAllInited(runMode);
  for (muse::modularity::IModuleSetup *m : m_modules)
    m->onAllInited(runMode);

  QMetaObject::invokeMethod(
      qApp,
      [this]
      {
        globalModule.onStartApp();
        for (muse::modularity::IModuleSetup *m : m_modules)
          m->onStartApp();
      },
      Qt::QueuedConnection);

  QQmlApplicationEngine *engine =
      ioc()->resolve<muse::ui::IUiEngine>("app")->qmlAppEngine();

  const QUrl url(QStringLiteral("qrc:/src/qml/Main.qml"));

  QObject::connect(
      engine, &QQmlApplicationEngine::objectCreated, qApp,
      [this, url](QObject *obj, const QUrl &objUrl)
      {
        if (!obj && url == objUrl)
        {
          LOGE() << "failed Qml load\n";
          QCoreApplication::exit(-1);
        }
        if (url == objUrl)
        {
          globalModule.onDelayedInit();
          for (muse::modularity::IModuleSetup *m : m_modules)
            m->onDelayedInit();
        }
      },
      Qt::QueuedConnection);

  QObject::connect(engine, &QQmlEngine::warnings,
                   [](const QList<QQmlError> &warnings)
                   {
                     for (const QQmlError &e : warnings)
                       LOGE()
                           << "error: " << e.toString().toStdString() << "\n";
                   });

  engine->load(url);

  qApp->exec();

  QThreadPool *globalThreadPool = QThreadPool::globalInstance();
  if (globalThreadPool)
  {
    LOGI() << "activeThreadCount: " << globalThreadPool->activeThreadCount();
    globalThreadPool->waitForDone();
  }

  globalModule.invokeQueuedCalls();

  for (muse::modularity::IModuleSetup *m : m_modules)
    m->onDeinit();

  globalModule.onDeinit();

  for (muse::modularity::IModuleSetup *m : m_modules)
    m->onDestroy();

  globalModule.onDestroy();

  // Delete modules
  qDeleteAll(m_modules);
  m_modules.clear();

  ioc()->reset();

  delete qApp;
}

muse::modularity::ModulesIoC *OrchestrionApp::ioc() const
{
  return muse::modularity::_ioc(m_context);
}

const muse::modularity::ContextPtr OrchestrionApp::iocContext() const
{
  return m_context;
}

QWindow *OrchestrionApp::focusWindow() const { return nullptr; }

bool OrchestrionApp::notify(QObject *, QEvent *) { return false; }

Qt::KeyboardModifiers OrchestrionApp::keyboardModifiers() const
{
  return QApplication::keyboardModifiers();
}

} // namespace dgk