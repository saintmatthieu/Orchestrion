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
#include "ExternalDevicesModule.h"
#include "internal/AudioDevice/AudioDeviceService.h"
#include "internal/ExternalDevicesConfiguration.h"
#include "internal/MidiDevice/MidiDeviceService.h"
#include <ui/iuiactionsregister.h>

namespace dgk
{
ExternalDevicesModule::ExternalDevicesModule()
    : m_audioDeviceService{std::make_shared<AudioDeviceService>()},
      m_midiDeviceService{std::make_shared<MidiDeviceService>()},
      m_configuration{std::make_shared<ExternalDevicesConfiguration>()}
{
}

std::string ExternalDevicesModule::moduleName() const { return "Orchestrion"; }

void ExternalDevicesModule::registerExports()
{
  ioc()->registerExport<IAudioDeviceService>(moduleName(),
                                             m_audioDeviceService);
  ioc()->registerExport<IMidiDeviceService>(moduleName(), m_midiDeviceService);
  ioc()->registerExport<IExternalDevicesConfiguration>(moduleName(),
                                                       m_configuration);
}

void ExternalDevicesModule::onInit(const muse::IApplication::RunMode &)
{
  m_audioDeviceService->init();
  m_midiDeviceService->init();
  m_configuration->init();
}

void ExternalDevicesModule::onDelayedInit()
{
  m_midiDeviceService->postInit();
  m_audioDeviceService->postInit();
}
} // namespace dgk