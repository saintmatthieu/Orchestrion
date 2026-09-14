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
#pragma once

#include "CommandOptions.h"
#include <global/internal/consoleapplication.h>

namespace dgk
{
/**
 * The headless Orchestrion. It runs the audio plugin registration the GUI
 * application delegates to a subprocess (a plugin that crashes while being
 * examined must not take the application down), then exits.
 */
class OrchestrionConsoleApp : public muse::ConsoleApplication
{
public:
  explicit OrchestrionConsoleApp(
      const std::shared_ptr<CommandOptions> &options);

  void showSplash() override;

private:
  void doStartupScenario(const muse::modularity::ContextPtr &ctx) override;
  int registerAudioPlugin(const CommandOptions::AudioPluginRegistration &task,
                          const muse::modularity::ContextPtr &ctx);
};
} // namespace dgk
