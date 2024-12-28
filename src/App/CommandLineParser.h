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
#pragma once

#include <QCommandLineParser>
#include <QStringList>

#include "CommandOptions.h"

class QCoreApplication;

namespace dgk
{
class CommandLineParser
{
  //! NOTE: This parser is created at the earliest stage of the application
  //! initialization You should not inject anything into it
public:
  CommandLineParser() = default;

  void init();
  void parse(int argc, char **argv);
  void processBuiltinArgs(const QCoreApplication &app);

  muse::IApplication::RunMode runMode() const;

  // CommandOptions
  const CommandOptions &options() const;

  // Tasks
  CommandOptions::ConverterTask converterTask() const;
  CommandOptions::Diagnostic diagnostic() const;
  CommandOptions::Autobot autobot() const;
  CommandOptions::AudioPluginRegistration audioPluginRegistration() const;

private:
  void printLongVersion() const;

  QCommandLineParser m_parser;
  CommandOptions m_options;
};
} // namespace dgk
