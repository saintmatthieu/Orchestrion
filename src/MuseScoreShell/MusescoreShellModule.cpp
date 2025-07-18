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
#include "MusescoreShellModule.h"
#include "view/OrchestrionMenuModel.h"

#include <appshell/view/framelesswindow/framelesswindowmodel.h>
#include <appshell/view/mainwindowtitleprovider.h>
#include <global/types/uri.h>
#include <ui/iinteractiveuriregister.h>
#include <ui/view/mainwindowbridge.h>

#include <QQmlEngine>

namespace dgk
{
std::string MusescoreShellModule::moduleName() const
{
  return "MusescoreShell";
}

void MusescoreShellModule::resolveImports()
{
  auto ir = ioc()->resolve<muse::ui::IInteractiveUriRegister>(moduleName());
  if (ir)
  {
    ir->registerUri(
        muse::Uri("musescore://notation"),
        muse::ui::ContainerMeta(muse::ui::ContainerType::PrimaryPage));
    ir->registerUri(muse::Uri("musescore://about/musescore"),
                    muse::ui::ContainerMeta(muse::ui::ContainerType::QmlDialog,
                                            "AboutDialog.qml"));
    ir->registerUri(muse::Uri("musescore://about/musicxml"),
                    muse::ui::ContainerMeta(muse::ui::ContainerType::QmlDialog,
                                            "AboutMusicXMLDialog.qml"));
  }
}

void MusescoreShellModule::registerUiTypes()
{
  qmlRegisterType<mu::appshell::MainWindowTitleProvider>(
      "MuseScore.AppShell", 1, 0, "MainWindowTitleProvider");
  qmlRegisterType<muse::ui::MainWindowBridge>("MuseScore.Ui", 1, 0,
                                              "MainWindowBridge");
  qmlRegisterType<mu::appshell::FramelessWindowModel>(
      "MuseScore.AppShell", 1, 0, "FramelessWindowModel");
  // Inject our OrchestrionMenuModel into the MuseScore OrchestrionShell
  // namespace
  qmlRegisterType<OrchestrionMenuModel>("MuseScore.AppShell", 1, 0,
                                        "AppMenuModel");
}
} // namespace dgk
