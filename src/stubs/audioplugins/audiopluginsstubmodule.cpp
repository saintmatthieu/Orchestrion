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
  Ret clear() override { return make_ok(); }
  AudioPluginInfoList pluginInfoList(PluginInfoAccepted) const override
  {
    return {};
  }
  async::Notification pluginInfoListChanged() const override
  {
    return m_pluginInfoListChanged;
  }
  const io::path_t &pluginPath(const PluginResourceId &) const override
  {
    static io::path_t emptyPath;
    return emptyPath;
  }
  bool exists(const io::path_t &) const override { return false; }
  bool exists(const PluginResourceId &) const override { return false; }
  Ret registerPlugins(const AudioPluginInfoList &) override
  {
    return make_ok();
  }
  Ret unregisterPlugins(const PluginResourceIdList &) override
  {
    return make_ok();
  }
  Ret setPluginsState(const io::paths_t &, AudioPluginState) override
  {
    return make_ok();
  }
  Ret removePluginsAtPath(const io::path_t &) override { return make_ok(); }
  Ret writePluginsTo(const io::path_t &,
                     const AudioPluginInfoList &) const override
  {
    return make_ok();
  }
  RetVal<AudioPluginInfoList> readPluginsFrom(const io::path_t &) const override
  {
    return RetVal<AudioPluginInfoList>::make_ok(AudioPluginInfoList{});
  }

private:
  async::Notification m_pluginInfoListChanged;
};
} // namespace

std::string AudioPluginsModule::moduleName() const
{
  return "audioplugins_stub";
}

void AudioPluginsModule::registerExports()
{
  globalIoc()->registerExport<IKnownAudioPluginsRegister>(
      moduleName(), new KnownAudioPluginsRegisterStub);
}
} // namespace muse::audioplugins
