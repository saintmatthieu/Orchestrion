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
#include "App/CommandLineParser.h"
#include "App/OrchestrionAppFactory.h"

#include "global/iapplication.h"
#include "log.h"
#include "muse_framework_config.h"

#include <csignal>

#if (defined(_MSCVER) || defined(_MSC_VER))
#include <algorithm>
// #include <shellapi.h>
#include <vector>
#include <windows.h>
#endif

#include <QApplication>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTextCodec>

#ifndef MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT
static void crashCallback(int signum)
{
  const char *signame = "UNKNOWN SIGNAME";
  const char *sigdescript = "";
  switch (signum)
  {
  case SIGILL:
    signame = "SIGILL";
    sigdescript = "Illegal Instruction";
    break;
  case SIGSEGV:
    signame = "SIGSEGV";
    sigdescript = "Invalid memory reference";
    break;
  }
  LOGE() << "Oops! Application crashed with signal: [" << signum << "] "
         << signame << "-" << sigdescript;
  exit(EXIT_FAILURE);
}

#endif

static void app_init_qrc()
{
  Q_INIT_RESOURCE(OrchestrionApp);
  Q_INIT_RESOURCE(OrchestrionShell);
  Q_INIT_RESOURCE(appshell);

#ifdef Q_OS_WIN
  // Q_INIT_RESOURCE(app_win);
#endif
}

int main(int argc, char **argv)
{
#ifndef MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT
  signal(SIGSEGV, crashCallback);
  signal(SIGILL, crashCallback);
  signal(SIGFPE, crashCallback);
#endif

  // ====================================================
  // Setup global Qt application variables
  // ====================================================

  // Force the 8-bit text encoding to UTF-8. This is the default encoding on all
  // supported platforms except for MSVC under Windows, which would otherwise
  // default to the local ANSI code page and cause corruption of any non-ANSI
  // Unicode characters in command-line arguments.
  QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

  app_init_qrc();

  qputenv("QT_STYLE_OVERRIDE", "Fusion");
  qputenv("QML_DISABLE_DISK_CACHE", "true");

#ifdef Q_OS_LINUX
  if (qEnvironmentVariable("QT_QPA_PLATFORM") != "offscreen")
  {
    qputenv("QT_QPA_PLATFORMTHEME", "gtk3");
  }
#endif

#ifdef Q_OS_WIN
  // NOTE: There are some problems with rendering the application window on some
  // integrated graphics processors
  //       see https://github.com/musescore/MuseScore/issues/8270
  if (!qEnvironmentVariableIsSet("QT_OPENGL_BUGLIST"))
  {
    qputenv("QT_OPENGL_BUGLIST", ":/resources/win_opengl_buglist.json");
  }
#endif

//! NOTE: For unknown reasons, Linux scaling for 1 is defined as 1.003 in
//! fractional scaling.
//!       Because of this, some elements are drawn with a shift on the score.
//!       Let's make a Linux hack and round values above 0.75(see
//!       RoundPreferFloor)
#ifdef Q_OS_LINUX
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor);
#elif defined(Q_OS_WIN)
  QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
      Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

  QGuiApplication::styleHints()->setMousePressAndHoldInterval(250);

  // Can't use "Orchestrion" until next major release, because this
  // "application name" is used to determine where user settings are stored.
  // Changing it would result in all user settings being lost.
#ifdef MUSE_APP_UNSTABLE
  QCoreApplication::setApplicationName("OrchestrionDevelopment");
#else
  QCoreApplication::setApplicationName("Orchestrion");
#endif
  QCoreApplication::setApplicationVersion("0.1.0");

#if (defined(_MSCVER) || defined(_MSC_VER))
  // On MSVC under Windows, we need to manually retrieve the command-line
  // arguments and convert them from UTF-16 to UTF-8. This prevents data loss if
  // there are any characters that wouldn't fit in the local ANSI code page.
  int argcUTF16 = 0;
  LPWSTR *argvUTF16 = CommandLineToArgvW(GetCommandLineW(), &argcUTF16);

  std::vector<QByteArray> argvUTF8Q;
  std::for_each(
      argvUTF16, argvUTF16 + argcUTF16,
      [&argvUTF8Q](const auto &arg)
      {
        argvUTF8Q.emplace_back(
            QString::fromUtf16(reinterpret_cast<const char16_t *>(arg), -1)
                .toUtf8());
      });

  LocalFree(argvUTF16);

  std::vector<char *> argvUTF8;
  for (auto &arg : argvUTF8Q)
  {
    argvUTF8.push_back(arg.data());
  }

  // Don't use the arguments passed to main(), because they're in the local ANSI
  // code page.
  Q_UNUSED(argc);
  Q_UNUSED(argv);

  int argcFinal = argcUTF16;
  char **argvFinal = argvUTF8.data();
#else

  int argcFinal = argc;
  char **argvFinal = argv;

#endif

  // ====================================================
  // Parse command line options
  // ====================================================
  dgk::CommandLineParser commandLineParser;
  commandLineParser.init();
  commandLineParser.parse(argcFinal, argvFinal);

  muse::IApplication::RunMode runMode = commandLineParser.runMode();
  QCoreApplication *qapp = nullptr;

  if (runMode == muse::IApplication::RunMode::AudioPluginRegistration)
    qapp = new QCoreApplication(argc, argv);
  else
    qapp = new QApplication(argc, argv);

  commandLineParser.processBuiltinArgs(*qapp);

  dgk::OrchestrionAppFactory f;
  std::shared_ptr<muse::IApplication> app =
      f.newApp(commandLineParser.options());

  app->perform();

  // ====================================================
  // Run main loop
  // ====================================================
  int code = qapp->exec();

  // ====================================================
  // Quit
  // ====================================================

  app->finish();

  LOGI() << "Goodbye!! code: " << code;
  return code;
}
