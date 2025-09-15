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
#include "touchpad_testapp_model.h"

#include <global/async/processevents.h>
#include <global/internal/invoker.h>

#include <QApplication>
#include <QQmlApplicationEngine>

int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  QQmlApplicationEngine engine;

  qmlRegisterType<dgk::TouchpadTestappModel>("Orchestrion.Test", 1, 0,
                                             "TouchpadTestappModel");
  qmlRegisterType<dgk::QContact>("Orchestrion.Test", 1, 0, "QContact");

  const auto url = QUrl(QStringLiteral("qrc:/touchpad_testapp_main.qml"));
  engine.load(url);

  const auto invoker = std::make_shared<muse::Invoker>();
  muse::async::onMainThreadInvoke(
      [&invoker](const std::function<void()> &f, bool isAlwaysQueued)
      { invoker->invoke(f, isAlwaysQueued); });

  return app.exec();
}
