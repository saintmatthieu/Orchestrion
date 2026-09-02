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

// Orchestrion modules
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

// Muse framework modules (and stubs for the ones Orchestrion doesn't build)
#include "muse_framework_config.h" // MUSE_MODULE_* macros
#include <framework/accessibility/accessibilitymodule.h>
#include <framework/actions/actionsmodule.h>
#include <framework/audio/main/audiomodule.h>
#include <framework/draw/drawmodule.h>
#include <framework/extensions/extensionsmodule.h>
#include <framework/interactive/interactivemodule.h>
#include <framework/languages/languagesmodule.h>
#include <framework/midi/midimodule.h>
#include <framework/mpe/mpemodule.h>
#include <framework/multiwindows/multiwindowsmodule.h>
#include <framework/network/networkmodule.h>
#include <framework/rcommand/rcommandmodule.h>
#include <framework/rcontrol/rcontrolmodule.h>
#include <framework/shortcuts/shortcutsmodule.h>
#ifdef MUSE_MODULE_AUTOMATION
#include <framework/automation/automationmodule.h>
#else
#include <framework/stubs/automation/automationstubmodule.h>
#endif
#ifdef MUSE_MODULE_CLOUD
#include <framework/cloud/cloudmodule.h>
#else
#include <framework/stubs/cloud/cloudstubmodule.h>
#endif
#ifdef MUSE_MODULE_LEARN
#include <framework/learn/learnmodule.h>
#else
#include <framework/stubs/learn/learnmodule.h>
#endif
#ifdef MUSE_MODULE_MEDIA
#include <framework/media/mediamodule.h>
#else
#include <framework/stubs/media/mediastubmodule.h>
#endif
#ifdef MUSE_MODULE_MIDIREMOTE
#include <framework/midiremote/midiremotemodule.h>
#else
#include <framework/stubs/midiremote/midiremotestubmodule.h>
#endif
#ifdef MUSE_MODULE_TOURS
#include <framework/tours/toursmodule.h>
#else
#include <framework/stubs/tours/toursstubmodule.h>
#endif
#ifdef MUSE_MODULE_UPDATE
#include <framework/update/updatemodule.h>
#else
#include <framework/stubs/update/updatestubmodule.h>
#endif
#include <framework/toast/toastmodule.h>
#include <framework/ui/uimodule.h>
#include <framework/uicomponents/uicomponentsmodule.h>
#include <framework/vst/vstmodule.h>
#include <framework/workspace/workspacemodule.h>
#ifdef MUSE_MODULE_DIAGNOSTICS
#include <framework/diagnostics/diagnosticsmodule.h>
#endif
#ifdef MUSE_MODULE_DOCKWINDOW
#include <framework/dockwindow/dockmodule.h>
#endif
#include <stubs/audioplugins/audiopluginsstubmodule.h> // Orchestrion's own stub

// MuseScore modules (and stubs)
#include <context/contextmodule.h>
#include <engraving/engravingmodule.h>
#include <importexport/midi/midimodule.h>
#include <importexport/musicxml/musicxmlmodule.h>
#include <instrumentsscene/instrumentsscenemodule.h>
#include <notation/notationmodule.h>
#include <notationscene/notationscenemodule.h>
#include <playback/playbackmodule.h>
#include <preferences/preferencesmodule.h>
#include <project/projectmodule.h>
#include <stubs/braille/braillestubmodule.h>
#include <stubs/importexport/mei/meimodule.h>
#include <stubs/importexport/mnx/mnxmodule.h>
#include <stubs/importexport/ove/ovemodule.h>
#include <stubs/musesounds/musesoundsstubmodule.h>
#include <stubs/palette/palettestubmodule.h>
#include <stubs/propertiespanel/propertiespanelstubmodule.h>

namespace dgk
{
std::shared_ptr<muse::IApplication> OrchestrionAppFactory::newApp(
    const std::shared_ptr<CommandOptions> &options) const
{
  if (options->runMode == muse::IApplication::RunMode::GuiApp)
    return newGuiApp(options);
  else
    return newConsoleApp(options);
}

std::shared_ptr<muse::IApplication> OrchestrionAppFactory::newGuiApp(
    const std::shared_ptr<CommandOptions> &options) const
{
  const auto app = std::make_shared<OrchestrionApp>(options);

#ifdef MUSE_MODULE_DIAGNOSTICS
  //! NOTE `diagnostics` must be first, because it installs the crash handler.
  app->addModule(new muse::diagnostics::DiagnosticsModule());
#endif

  // framework
  app->addModule(new muse::accessibility::AccessibilityModule());
  app->addModule(new muse::actions::ActionsModule());
  app->addModule(new muse::rcommand::RCommandModule());
  app->addModule(new muse::rcontrol::RControlModule());
  app->addModule(new muse::audio::AudioModule());
  app->addModule(new muse::audioplugins::AudioPluginsModule());
  app->addModule(new muse::automation::AutomationModule());
  app->addModule(new muse::draw::DrawModule());
  app->addModule(new muse::interactive::InteractiveModule());
  app->addModule(new muse::midi::MidiModule());
  app->addModule(new muse::midiremote::MidiRemoteModule());
  app->addModule(new muse::mpe::MpeModule());
  app->addModule(new muse::network::NetworkModule());
  app->addModule(new muse::shortcuts::ShortcutsModule());
  app->addModule(new muse::ui::UiModule());
  app->addModule(new muse::uicomponents::UiComponentsModule());
#ifdef MUSE_MODULE_DOCKWINDOW
  app->addModule(new muse::dock::DockModule());
#endif
  app->addModule(new muse::toast::ToastModule());
  app->addModule(new muse::tours::ToursModule());
  app->addModule(new muse::vst::VSTModule());
  app->addModule(new muse::media::MediaModule());

  // MuseScore
  app->addModule(new mu::braille::BrailleModule());
  app->addModule(new muse::cloud::CloudModule());
  app->addModule(new mu::context::ContextModule());
  app->addModule(new mu::engraving::EngravingModule());
  app->addModule(new mu::iex::midi::MidiModule());
  app->addModule(new mu::iex::mnxio::MnxModule());
  app->addModule(new mu::iex::musicxml::MusicXmlModule());
  app->addModule(new mu::iex::ove::OveModule());
  app->addModule(new mu::iex::mei::MeiModule());
  app->addModule(new mu::propertiespanel::PropertiesPanelModule());
  app->addModule(new mu::instrumentsscene::InstrumentsSceneModule());
  app->addModule(new muse::extensions::ExtensionsModule());
  app->addModule(new muse::languages::LanguagesModule());
  app->addModule(new muse::learn::LearnModule());
  app->addModule(new muse::mi::MultiWindowsModule());
  app->addModule(new mu::musesounds::MuseSoundsModule());
  app->addModule(new mu::notation::NotationModule());
  app->addModule(new mu::notation::NotationSceneModule());
  app->addModule(new mu::palette::PaletteModule());
  app->addModule(new mu::playback::PlaybackModule());
  app->addModule(new mu::preferences::PreferencesModule());
  app->addModule(new mu::project::ProjectModule());
  app->addModule(new muse::update::UpdateModule());
  app->addModule(new muse::workspace::WorkspaceModule());

  // Orchestrion
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

  return app;
}

std::shared_ptr<muse::IApplication> OrchestrionAppFactory::newConsoleApp(
    const std::shared_ptr<CommandOptions> &) const
{
  // For now
  return nullptr;
}
} // namespace dgk
