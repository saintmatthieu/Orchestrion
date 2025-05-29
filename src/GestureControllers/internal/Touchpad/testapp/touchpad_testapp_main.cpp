#include "touchpad_testapp_model.h"

#include <async/processevents.h>
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
