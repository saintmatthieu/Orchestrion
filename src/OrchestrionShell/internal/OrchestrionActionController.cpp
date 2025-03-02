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
#include "OrchestrionActionController.h"
#include <engraving/dom/masterscore.h>
#include <notation/imasternotation.h>

namespace dgk
{
void OrchestrionActionController::init()
{
  dispatcher()->reg(this, "orchestrion-file-open",
                    [this]
                    {
                      if (globalContext()->currentProject())
                        dispatcher()->dispatch("orchestrion-file-close");
                      dispatcher()->dispatch("file-open");
                    });

  dispatcher()->reg(this, "orchestrion-file-close",
                    [this]
                    {
                      if (const auto notation =
                              globalContext()->currentMasterNotation())
                        notation->masterScore()->setSaved(true);
                      dispatcher()->dispatch("file-close");
                    });
}
} // namespace dgk