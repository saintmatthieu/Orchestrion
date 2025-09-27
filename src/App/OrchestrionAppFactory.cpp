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
#include "OrchestrionAppFactory.h"
#include "OrchestrionApp.h"

#include "ExternalDevices/ExternalDevicesModule.h"
#include "GestureControllers/GestureControllersModule.h"
#include "MuseScoreShell/MusescoreShellModule.h"
#include "OrchestrionConfiguration/OrchestrionConfigurationModule.h"
#include "OrchestrionNotation/OrchestrionNotationModule.h"
#include "OrchestrionOnboarding/OrchestrionOnboardingModule.h"
#include "OrchestrionSequencer/OrchestrionModule.h"
#include "OrchestrionShell/OrchestrionShellModule.h"
#include "OrchestrionSynthesis/OrchestrionSynthesisModule.h"
#include "ScoreAnimation/ScoreAnimationModule.h"

#include <accessibility/accessibilitymodule.h>
#include <actions/actionsmodule.h>
#include <audio/main/audiomodule.h>
#include <commonscene/commonscenemodule.h>
#include <context/contextmodule.h>
#include <draw/drawmodule.h>
#include <engraving/engravingmodule.h>
#include <extensions/extensionsmodule.h>
#include <global/globalmodule.h>
#include <importexport/midi/midimodule.h>
#include <importexport/musicxml/musicxmlmodule.h>
#include <languages/languagesmodule.h>
#include <midi/midimodule.h>
#include <mpe/mpemodule.h>
#include <multiinstances/multiinstancesmodule.h>
#include <network/networkmodule.h>
#include <notation/notationmodule.h>
#include <playback/playbackmodule.h>
#include <project/projectmodule.h>
#include <shortcuts/shortcutsmodule.h>
#include <stubs/audioplugins/audiopluginsstubmodule.h>
#include <stubs/musesounds/musesoundsstubmodule.h>
#include <tours/toursmodule.h>
#include <ui/uimodule.h>
#include <uicomponents/uicomponentsmodule.h>
#include <vst/vstmodule.h>
#include <workspace/workspacemodule.h>

#ifdef MUSE_MODULE_UPDATE
#include <update/updatemodule.h>
#else
#include <stubs/update/updatestubmodule.h>
#endif

namespace dgk
{
std::shared_ptr<muse::IApplication>
OrchestrionAppFactory::newApp(const CommandOptions &options) const
{
  if (options.runMode == muse::IApplication::RunMode::GuiApp)
    return newGuiApp(options);
  else
    return newConsoleApp(options);
}

std::shared_ptr<muse::IApplication>
OrchestrionAppFactory::newGuiApp(const CommandOptions &options) const
{
  muse::modularity::ContextPtr ctx =
      std::make_shared<muse::modularity::Context>();
  ++m_lastID;
  // ctx->id = m_lastID;
  ctx->id = -1; //! NOTE At the moment global ioc

  const auto app = std::make_shared<OrchestrionApp>(options);

  app->addModule(new MusescoreShellModule());
  app->addModule(new OrchestrionShellModule());
  app->addModule(new OrchestrionSynthesisModule());
  app->addModule(new OrchestrionNotationModule());
  app->addModule(new OrchestrionOnboardingModule());
  app->addModule(new OrchestrionModule());
  app->addModule(new ScoreAnimationModule());
  app->addModule(new ExternalDevicesModule());
  app->addModule(new GestureControllersModule());
  app->addModule(new OrchestrionConfigurationModule());

  app->addModule(new muse::accessibility::AccessibilityModule());
  app->addModule(new muse::actions::ActionsModule());
  app->addModule(new muse::audio::AudioModule());
  app->addModule(new muse::audioplugins::AudioPluginsModule());
  app->addModule(new mu::musesounds::MuseSoundsModule());
  app->addModule(new mu::commonscene::CommonSceneModule());
  app->addModule(new mu::context::ContextModule());
  app->addModule(new muse::tours::ToursModule());
  app->addModule(new muse::draw::DrawModule());
  app->addModule(new mu::engraving::EngravingModule());
  app->addModule(new muse::extensions::ExtensionsModule());
  app->addModule(new muse::languages::LanguagesModule());
  app->addModule(new mu::iex::midi::MidiModule());
  app->addModule(new mu::iex::musicxml::MusicXmlModule());
  app->addModule(new muse::midi::MidiModule());
  app->addModule(new muse::mpe::MpeModule());
  app->addModule(new muse::mi::MultiInstancesModule());
  app->addModule(new muse::network::NetworkModule());
  app->addModule(new mu::notation::NotationModule());
  app->addModule(new mu::playback::PlaybackModule());
  app->addModule(new mu::project::ProjectModule());
  app->addModule(new muse::shortcuts::ShortcutsModule());
  app->addModule(new muse::ui::UiModule());
  app->addModule(new muse::uicomponents::UiComponentsModule());
  app->addModule(new muse::update::UpdateModule());
  app->addModule(new muse::vst::VSTModule());
  app->addModule(new muse::workspace::WorkspaceModule());

  return app;
}

std::shared_ptr<muse::IApplication>
OrchestrionAppFactory::newConsoleApp(const CommandOptions &) const
{
  // For now
  return nullptr;
}

} // namespace dgk