/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "audiopluginsstubmodule.h"
#include "audioplugins/iknownaudiopluginsregister.h"

namespace muse::audioplugins
{
namespace
{
class KnownAudioPluginsRegisterStub : public IKnownAudioPluginsRegister
{
public:
  Ret load() override { return make_ok(); }

  std::vector<AudioPluginInfo>
  pluginInfoList(PluginInfoAccepted accepted) const override
  {
    return {};
  }

  const io::path_t &pluginPath(const audio::AudioResourceId &) const override
  {
    static io::path_t emptyPath;
    return emptyPath;
  }

  bool exists(const io::path_t &) const override { return false; }

  bool exists(const audio::AudioResourceId &) const override { return false; }

  Ret registerPlugin(const AudioPluginInfo &) override { return make_ok(); }

  Ret unregisterPlugin(const audio::AudioResourceId &) override
  {
    return make_ok();
  }
};
} // namespace

std::string AudioPluginsModule::moduleName() const
{
  return "audioplugins_stub";
}

void AudioPluginsModule::registerExports()
{
  ioc()->registerExport<IKnownAudioPluginsRegister>(
      moduleName(), new KnownAudioPluginsRegisterStub);
}
} // namespace muse::audioplugins