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

#include "internal/IExternalDeviceService.h"

#include "async/notification.h"
#include "modularity/ioc.h"
#include <optional>

namespace dgk
{
class IMidiDeviceService : public IExternalDeviceService,
                           MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IMidiDeviceService);

public:
  virtual ~IMidiDeviceService() = default;
  virtual muse::async::Notification startupSelectionFinished() const = 0;
};
} // namespace dgk