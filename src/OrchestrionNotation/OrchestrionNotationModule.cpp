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
#include "OrchestrionNotationModule.h"
#include "internal/OrchestrionNotationInteractionProcessor.h"
#include "view/OrchestrionNotationPaintView.h"

namespace dgk
{
std::string OrchestrionNotationModule::moduleName() const
{
  return "OrchestrionNotation";
}

void OrchestrionNotationModule::registerUiTypes()
{
  qmlRegisterType<OrchestrionNotationPaintView>(
      "Orchestrion.OrchestrionNotation", 1, 0, "OrchestrionNotationPaintView");
}

void OrchestrionNotationModule::registerExports()
{
  ioc()->registerExport<IOrchestrionNotationInteractionProcessor>(
      moduleName(),
      std::make_shared<OrchestrionNotationInteractionProcessor>());
}
} // namespace dgk