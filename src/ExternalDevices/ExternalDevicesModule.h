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

#include "modularity/imodulesetup.h"
#include <memory>

namespace dgk
{
class ExternalDevicesConfiguration;
class AudioDeviceService;
class MidiDeviceService;

class ExternalDevicesModule : public muse::modularity::IModuleSetup
{
public:
  ExternalDevicesModule();

private:
  std::string moduleName() const override;
  void registerExports() override;
  muse::modularity::IContextSetup *
  newContext(const muse::modularity::ContextPtr &ctx) const override;

  void onContextInit(const muse::IApplication::RunMode &) const;
  void onContextAllInited(const muse::IApplication::RunMode &) const;

  const std::shared_ptr<AudioDeviceService> m_audioDeviceService;
  const std::shared_ptr<MidiDeviceService> m_midiDeviceService;
  const std::shared_ptr<ExternalDevicesConfiguration> m_configuration;
};
} // namespace dgk
