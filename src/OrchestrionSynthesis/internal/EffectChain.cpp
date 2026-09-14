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
#include "EffectChain.h"
#include "BuiltInEffectResources.h"
#include "OrchestrionFxResolver.h"
#include <audio/common/audioutils.h>
#include <global/settings.h>
#include <global/translation.h>
#include <log.h>
#include <vst/vstpluginattrs.h>
#include <vst/ivstplugininstance.h>

#include <QGuiApplication>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace dgk
{
namespace
{
const muse::Settings::Key MASTER_EFFECT_CHAIN("OrchestrionSynthesis",
                                              "MASTER_EFFECT_CHAIN");
// The setting's value until a chain is first saved: the built-in effects go
// in (a saved empty chain is "[]").
const std::string defaultChainMarker("default");
// The built-in effects' window (see BuiltInEffectDialog.qml).
const std::string builtInEffectDialogUri("orchestrion://effects/builtin");

// The VST module's action that opens a plugin's editor window (see
// muse::vst::VstActionsController).
const std::string fxEditorAction("action://vst/fx_editor");

bool isVstEffect(const muse::audioplugins::AudioPluginInfo &info)
{
  return info.meta.type == muse::vst::AUDIO_RESOURCE_TYPE_NAME &&
         info.state == muse::audioplugins::AudioPluginState::Validated &&
         !info.meta.attributeVal(muse::vst::CATEGORIES_ATTRIBUTE)
              .contains(muse::vst::INSTRUMENT_CATEGORY);
}

// The plugin registry's meta and the audio engine's are twins; bridge them
// field-wise (as muse::vst::VstModulesRepository does).
muse::audio::AudioResourceMeta
toResourceMeta(const muse::audioplugins::PluginMeta &meta)
{
  muse::audio::AudioResourceMeta result;
  result.id = meta.id;
  result.vendor = meta.vendor;
  result.attributes = meta.attributes;
  result.type = meta.type;
  return result;
}

// A VST plugin's resource id is its name; the built-in effects have theirs.
std::string displayName(const muse::audio::AudioResourceMeta &meta)
{
  if (const std::optional<BuiltInEffect> effect = builtInEffectOf(meta.id))
    return builtInEffectName(*effect);
  return meta.id;
}

EffectDesc describe(const muse::audio::AudioResourceMeta &meta,
                    bool active = true)
{
  return {meta.id, displayName(meta), meta.vendor,
          builtInEffectOf(meta.id).has_value(), active};
}

muse::audio::AudioFxParams
makeParams(const muse::audio::AudioResourceMeta &meta,
           muse::audio::AudioFxChainOrder order)
{
  muse::audio::AudioFxParams params;
  params.resourceMeta = meta;
  params.categories = muse::audio::audioFxCategoriesFromString(
      meta.attributeVal(muse::vst::CATEGORIES_ATTRIBUTE));
  params.active = true;
  params.chainOrder = order;
  return params;
}

// What a new user gets: compression, then the room, then the safety net.
muse::audio::AudioFxChain defaultChain()
{
  muse::audio::AudioFxChain chain;
  muse::audio::AudioFxChainOrder order = 0;
  for (const auto effect : {BuiltInEffect::Compressor, BuiltInEffect::Reverb,
                            BuiltInEffect::Limiter})
  {
    chain.emplace(order, makeParams(builtInEffectMeta(effect), order));
    ++order;
  }
  return chain;
}

QJsonObject attributesToJson(const muse::audio::AudioResourceAttributes &attrs)
{
  QJsonObject result;
  for (const auto &[key, value] : attrs)
    result.insert(key.toQString(), value.toQString());
  return result;
}

muse::audio::AudioResourceAttributes attributesFromJson(const QJsonObject &obj)
{
  muse::audio::AudioResourceAttributes result;
  for (const QString &key : obj.keys())
    result.emplace(muse::String::fromQString(key),
                   muse::String::fromQString(obj.value(key).toString()));
  return result;
}

QJsonObject resourceMetaToJson(const muse::audio::AudioResourceMeta &meta)
{
  QJsonObject result;
  result.insert("id", QString::fromStdString(meta.id));
  result.insert("vendor", QString::fromStdString(meta.vendor));
  result.insert("type", QString::fromStdString(meta.type));
  result.insert("attributes", attributesToJson(meta.attributes));
  return result;
}

muse::audio::AudioResourceMeta resourceMetaFromJson(const QJsonObject &obj)
{
  muse::audio::AudioResourceMeta result;
  result.id = obj.value("id").toString().toStdString();
  result.vendor = obj.value("vendor").toString().toStdString();
  result.type = obj.value("type").toString().toStdString();
  result.attributes = attributesFromJson(obj.value("attributes").toObject());
  return result;
}

// The plugin state is binary: base64 in the JSON (as MuseScore's project
// audio settings do).
QJsonObject configurationToJson(const muse::audio::AudioUnitConfig &config)
{
  QJsonObject result;
  for (const auto &[key, value] : config)
  {
    const QByteArray bytes = QByteArray::fromRawData(
        value.c_str(), static_cast<qsizetype>(value.size()));
    result.insert(QString::fromStdString(key), QString(bytes.toBase64()));
  }
  return result;
}

muse::audio::AudioUnitConfig configurationFromJson(const QJsonObject &obj)
{
  muse::audio::AudioUnitConfig result;
  for (const QString &key : obj.keys())
  {
    const QByteArray bytes =
        QByteArray::fromBase64(obj.value(key).toString().toUtf8());
    result.emplace(key.toStdString(), bytes.toStdString());
  }
  return result;
}

QJsonObject fxParamsToJson(const muse::audio::AudioFxParams &params)
{
  QJsonObject result;
  result.insert("resourceMeta", resourceMetaToJson(params.resourceMeta));
  result.insert("active", params.active);
  result.insert("configuration", configurationToJson(params.configuration));
  return result;
}

muse::audio::AudioFxParams fxParamsFromJson(const QJsonObject &obj,
                                            muse::audio::AudioFxChainOrder order)
{
  muse::audio::AudioFxParams result;
  result.resourceMeta = resourceMetaFromJson(obj.value("resourceMeta").toObject());
  result.active = obj.value("active").toBool(true);
  result.configuration =
      configurationFromJson(obj.value("configuration").toObject());
  result.chainOrder = order;
  result.categories = muse::audio::audioFxCategoriesFromString(
      result.resourceMeta.attributeVal(muse::vst::CATEGORIES_ATTRIBUTE));
  return result;
}

// The chain as a JSON array, in processing order.
std::string chainToJson(const muse::audio::AudioFxChain &chain)
{
  QJsonArray array;
  for (const auto &[order, params] : chain)
    array.append(fxParamsToJson(params));
  return QJsonDocument(array).toJson(QJsonDocument::Compact).toStdString();
}

muse::audio::AudioFxChain chainFromJson(const std::string &json)
{
  muse::audio::AudioFxChain chain;
  const QJsonDocument doc =
      QJsonDocument::fromJson(QByteArray::fromStdString(json));
  if (!doc.isArray())
    return chain;
  muse::audio::AudioFxChainOrder order = 0;
  for (const QJsonValue value : doc.array())
  {
    if (!value.isObject())
      continue;
    muse::audio::AudioFxParams params = fxParamsFromJson(value.toObject(), order);
    if (params.resourceMeta.id.empty())
      continue;
    chain.emplace(order, std::move(params));
    ++order;
  }
  return chain;
}

// Same effects at the same positions (whatever their state).
bool sameLayout(const muse::audio::AudioFxChain &a,
                const muse::audio::AudioFxChain &b)
{
  return a.size() == b.size() &&
         std::equal(a.begin(), a.end(), b.begin(),
                    [](const auto &x, const auto &y)
                    {
                      return x.first == y.first &&
                             x.second.resourceMeta.id ==
                                 y.second.resourceMeta.id;
                    });
}
} // namespace

EffectChain::EffectChain(std::shared_ptr<OrchestrionFxResolver> resolver)
    : m_resolver{std::move(resolver)}
{
}

void EffectChain::registerResolver()
{
  fxResolver()->registerResolver(muse::audio::AudioFxType::MuseFx, m_resolver);
}

void EffectChain::onAllInited()
{
  load();

  // Like the synthesizer resolver: registered now, and again when the audio
  // starts, in case the engine's setup put the built-in one back.
  registerResolver();
  startAudioController()->isAudioStartedChanged().onReceive(
      this,
      [this](bool started)
      {
        if (started)
          registerResolver();
      });

  // On every project load, MuseScore's playback controller sets the master
  // chain from the project's own audio settings; play becoming allowed comes
  // after that, so this is where ours goes back in.
  playbackController()->isPlayAllowedChanged().onReceive(
      this,
      [this](bool allowed)
      {
        if (allowed)
          apply();
      });

  playback()->masterFxChainParamsChanged().onReceive(
      this, [this](const muse::audio::AudioFxChain &chain)
      { onEngineChainChanged(chain); });

  knownPlugins()->pluginInfoListChanged().onNotify(
      this, [this] { m_availableEffectsChanged.notify(); });
}

std::vector<muse::audioplugins::AudioPluginInfo>
EffectChain::availablePlugins() const
{
  return knownPlugins()->pluginInfoList(isVstEffect);
}

std::vector<EffectDesc> EffectChain::availableEffects() const
{
  std::vector<EffectDesc> result;
  for (const muse::audioplugins::AudioPluginInfo &info : availablePlugins())
    result.push_back(describe(toResourceMeta(info.meta)));
  std::sort(result.begin(), result.end(),
            [](const EffectDesc &a, const EffectDesc &b)
            { return a.name < b.name; });
  // The built-in ones first.
  std::vector<EffectDesc> builtIn;
  for (const auto effect : allBuiltInEffects)
    builtIn.push_back(describe(builtInEffectMeta(effect)));
  result.insert(result.begin(), builtIn.begin(), builtIn.end());
  return result;
}

muse::async::Notification EffectChain::availableEffectsChanged() const
{
  return m_availableEffectsChanged;
}

std::vector<EffectDesc> EffectChain::chain() const
{
  std::vector<EffectDesc> result;
  for (const auto &[order, params] : m_chain)
    result.push_back(describe(params.resourceMeta, params.active));
  return result;
}

muse::async::Notification EffectChain::chainChanged() const
{
  return m_chainChanged;
}

bool EffectChain::contains(const std::string &effectId) const
{
  return std::any_of(m_chain.begin(), m_chain.end(),
                     [&effectId](const auto &entry)
                     { return entry.second.resourceMeta.id == effectId; });
}

void EffectChain::addEffect(const std::string &effectId)
{
  if (contains(effectId))
    return;
  std::optional<muse::audio::AudioResourceMeta> meta;
  if (const std::optional<BuiltInEffect> effect = builtInEffectOf(effectId))
    meta = builtInEffectMeta(*effect);
  else
  {
    const auto plugins = availablePlugins();
    const auto it = std::find_if(
        plugins.begin(), plugins.end(),
        [&effectId](const muse::audioplugins::AudioPluginInfo &info)
        { return info.meta.id == effectId; });
    if (it != plugins.end())
      meta = toResourceMeta(it->meta);
  }
  if (!meta)
  {
    LOGW() << "Unknown effect: " << effectId;
    return;
  }
  const auto order = static_cast<muse::audio::AudioFxChainOrder>(m_chain.size());
  m_chain.emplace(order, makeParams(*meta, order));
  onChainModified();
  openEditorWhenReady(effectId);
}

void EffectChain::removeEffect(const std::string &effectId)
{
  // Keep the positions contiguous: the following effects move up.
  muse::audio::AudioFxChain remaining;
  muse::audio::AudioFxChainOrder order = 0;
  bool found = false;
  for (const auto &[_, params] : m_chain)
  {
    if (params.resourceMeta.id == effectId)
    {
      found = true;
      continue;
    }
    muse::audio::AudioFxParams moved = params;
    moved.chainOrder = order;
    remaining.emplace(order, std::move(moved));
    ++order;
  }
  if (!found)
    return;
  m_chain = std::move(remaining);
  onChainModified();
}

bool EffectChain::isActive(const std::string &effectId) const
{
  const auto it = std::find_if(m_chain.begin(), m_chain.end(),
                               [&effectId](const auto &entry)
                               { return entry.second.resourceMeta.id == effectId; });
  return it != m_chain.end() && it->second.active;
}

void EffectChain::setActive(const std::string &effectId, bool active)
{
  const auto it = std::find_if(m_chain.begin(), m_chain.end(),
                               [&effectId](const auto &entry)
                               { return entry.second.resourceMeta.id == effectId; });
  if (it == m_chain.end() || it->second.active == active)
    return;
  it->second.active = active;
  onChainModified();
}

void EffectChain::openEditorWhenReady(const std::string &effectId, int attempt)
{
  if (builtInEffectOf(effectId))
  {
    openEditor(effectId);
    return;
  }
  constexpr int intervalMs = 200;
  constexpr int maxAttempts = 25;
  const auto it = std::find_if(m_chain.begin(), m_chain.end(),
                               [&effectId](const auto &entry)
                               { return entry.second.resourceMeta.id == effectId; });
  if (it == m_chain.end() || attempt > maxAttempts)
    return;
  // The engine creates the instance on its own thread, and it loads on the
  // main thread afterwards: there is nothing to find before the first
  // interval has passed (and the register complains when asked too early).
  if (attempt > 0)
  {
    const muse::vst::IVstPluginInstancePtr instance =
        vstInstancesRegister()->fxPlugin(
            effectId, static_cast<int>(muse::audio::MASTER_TRACK_ID),
            static_cast<int>(it->first));
    if (instance && instance->isLoaded())
    {
      openEditor(effectId);
      return;
    }
  }
  QTimer::singleShot(intervalMs, [this, effectId, attempt]
                     { openEditorWhenReady(effectId, attempt + 1); });
}

void EffectChain::openEditor(const std::string &effectId)
{
  const auto it = std::find_if(m_chain.begin(), m_chain.end(),
                               [&effectId](const auto &entry)
                               { return entry.second.resourceMeta.id == effectId; });
  if (it == m_chain.end())
    return;
  if (const std::optional<BuiltInEffect> effect = builtInEffectOf(effectId))
  {
    muse::UriQuery query(builtInEffectDialogUri);
    query.addParam("effect", muse::Val(std::string(builtInEffectKey(*effect))));
    query.addParam("modal", muse::Val(false));
    interactive()->open(query);
    return;
  }
#ifdef Q_OS_LINUX
  // The VST module embeds the editor through an X11 window id; on another Qt
  // platform (wayland) the plugin's X11 GUI dies on a BadWindow error and
  // takes the application with it. main.cpp forces xcb, so this only guards
  // an explicit QT_QPA_PLATFORM.
  if (QGuiApplication::platformName() != QLatin1String("xcb"))
  {
    interactive()->warning(
        muse::trc("effects", "Plugin editors need X11"),
        muse::IInteractive::Text(muse::trc(
            "effects", "Start Orchestrion with QT_QPA_PLATFORM=xcb to open "
                       "a plugin's settings window.")));
    return;
  }
#endif
  // The master chain is the engine's track MASTER_TRACK_ID, which is how the
  // VST module keys its instances (MuseScore's mixer opens it the same way).
  // Checked first: the VST module asserts on a missing instance (a plugin
  // that failed to load), which would take a debug build down.
  const int trackId = static_cast<int>(muse::audio::MASTER_TRACK_ID);
  const int chainOrder = static_cast<int>(it->first);
  if (!vstInstancesRegister()->fxPlugin(effectId, trackId, chainOrder))
  {
    LOGW() << "No plugin instance for effect " << effectId
           << ": nothing to edit";
    return;
  }
  muse::actions::ActionQuery query(fxEditorAction);
  query.addParam("resourceId", muse::Val(effectId));
  query.addParam("trackId", muse::Val(trackId));
  query.addParam("chainOrder", muse::Val(chainOrder));
  dispatcher()->dispatch(query);
}

void EffectChain::onEngineChainChanged(const muse::audio::AudioFxChain &chain)
{
  if (!sameLayout(chain, m_chain))
    // Not ours: MuseScore's (from a project's audio settings; ours goes back
    // in when play becomes allowed), or ours minus the effects the engine
    // could not create (a plugin gone missing). Neither is worth keeping.
    return;
  if (chain == m_chain)
    return;
  // A plugin's state was changed in its editor: remember it.
  m_chain = chain;
  save();
}

void EffectChain::onChainModified()
{
  save();
  apply();
  m_chainChanged.notify();
}

void EffectChain::apply() { playback()->setMasterFxChainParams(m_chain); }

void EffectChain::load()
{
  muse::settings()->setDefaultValue(MASTER_EFFECT_CHAIN,
                                    muse::Val{defaultChainMarker});
  const std::string saved =
      muse::settings()->value(MASTER_EFFECT_CHAIN).toString();
  m_chain = saved == defaultChainMarker ? defaultChain() : chainFromJson(saved);
}

void EffectChain::save() const
{
  muse::settings()->setLocalValue(MASTER_EFFECT_CHAIN,
                                  muse::Val{chainToJson(m_chain)});
}
} // namespace dgk
