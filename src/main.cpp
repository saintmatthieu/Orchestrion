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
#include "app/app.h"

#include "MuseScoreShell/MusescoreShellModule.h"
#include "OrchestrionShell/OrchestrionShellModule.h"

#include "accessibility/accessibilitymodule.h"
#include "actions/actionsmodule.h"
#include "audio/audiomodule.h"
#include "braille/braillemodule.h"
#include "commonscene/commonscenemodule.h"
#include "context/contextmodule.h"
#include "diagnostics/diagnosticsmodule.h"
#include "draw/drawmodule.h"
#include "engraving/engravingmodule.h"
#include "extensions/extensionsmodule.h"
#include "midi/midimodule.h"
#include "mpe/mpemodule.h"
#include "multiinstances/multiinstancesmodule.h"
#include "notation/notationmodule.h"
#include "playback/playbackmodule.h"
#include "project/projectmodule.h"
#include "shortcuts/shortcutsmodule.h"
#include "ui/uimodule.h"
#include "uicomponents/uicomponentsmodule.h"
#include "workspace/workspacemodule.h"

int main(int argc, char *argv[])
{
  dgk::orchestrion::App app;

  app.addModule(new dgk::orchestrion::MusescoreShellModule());
  app.addModule(new dgk::orchestrion::OrchestrionShellModule());

  app.addModule(new muse::accessibility::AccessibilityModule());
  app.addModule(new muse::actions::ActionsModule());
  app.addModule(new muse::audio::AudioModule());
  app.addModule(new mu::braille::BrailleModule());
  app.addModule(new mu::commonscene::CommonSceneModule());
  app.addModule(new mu::context::ContextModule());
  app.addModule(new mu::diagnostics::DiagnosticsModule());
  app.addModule(new muse::draw::DrawModule());
  app.addModule(new mu::engraving::EngravingModule());
  app.addModule(new muse::extensions::ExtensionsModule());
  app.addModule(new muse::midi::MidiModule());
  app.addModule(new muse::mpe::MpeModule());
  app.addModule(new muse::mi::MultiInstancesModule());
  app.addModule(new mu::notation::NotationModule());
  app.addModule(new mu::playback::PlaybackModule());
  app.addModule(new mu::project::ProjectModule());
  app.addModule(new muse::shortcuts::ShortcutsModule());
  app.addModule(new muse::ui::UiModule());
  app.addModule(new muse::uicomponents::UiComponentsModule());
  app.addModule(new muse::workspace::WorkspaceModule());
  return app.run(argc, argv);
}
