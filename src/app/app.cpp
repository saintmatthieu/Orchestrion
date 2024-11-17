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

#include "app.h"

#include <global/globalmodule.h>
#include <modularity/ioc.h>
#include <ui/iuiengine.h>

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QThreadPool>

#include <future>

static muse::GlobalModule globalModule;

namespace dgk::orchestrion
{
App::App() : m_context(std::make_shared<muse::modularity::Context>()) {}

void App::addModule(muse::modularity::IModuleSetup *module)
{
  m_modules.push_back(module);
};

int App::run(int argc, char **argv)
{
  Q_INIT_RESOURCE(resources);
  const auto qapp = new QApplication(argc, argv);
  QCoreApplication::setApplicationName("Orchestrion");
  QCoreApplication::setApplicationVersion("0.1.0");

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

  const auto config = projectConfiguration();
  auto options = config->migrationOptions(mu::project::MigrationType::Ver_3_6);
  options.isAskAgain = false;
  config->setMigrationOptions(mu::project::MigrationType::Ver_3_6, options);

  QQmlApplicationEngine *engine =
      ioc()->resolve<muse::ui::IUiEngine>("app")->qmlAppEngine();

  const QUrl url(QStringLiteral("qrc:/src/qml/Main.qml"));

  QObject::connect(
      engine, &QQmlApplicationEngine::objectCreated, qapp,
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

  const auto retCode = qapp->exec();

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

  delete qapp;

  return retCode;
}

muse::modularity::ModulesIoC *App::ioc() const
{
  return muse::modularity::_ioc(m_context);
}

const muse::modularity::ContextPtr App::iocContext() const { return m_context; }

QWindow *App::focusWindow() const { return nullptr; }

bool App::notify(QObject *, QEvent *) { return false; }

} // namespace dgk::orchestrion