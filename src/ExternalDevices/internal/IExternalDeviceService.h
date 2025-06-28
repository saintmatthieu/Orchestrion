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

#include "../ExternalDevicesTypes.h"

#include "async/notification.h"
#include <optional>
#include <vector>

namespace dgk
{
class IExternalDeviceService
{
public:
  virtual ~IExternalDeviceService() = default;

  // Returns the names of the available devices.
  virtual std::vector<ExternalDeviceId> availableDevices() const = 0;
  virtual muse::async::Notification availableDevicesChanged() const = 0;
  virtual bool isAvailable(const ExternalDeviceId &) const = 0;
  virtual bool isNoDevice(const ExternalDeviceId &id) const = 0;

  // Does not have to be available.
  virtual void selectDevice(const std::optional<ExternalDeviceId> &) = 0;
  virtual muse::async::Notification selectedDeviceChanged() const = 0;
  virtual std::optional<ExternalDeviceId> selectedDevice() const = 0;

  virtual void selectDefaultDevice() = 0;

  virtual std::string deviceName(const ExternalDeviceId &) const = 0;
};
} // namespace dgk