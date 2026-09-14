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
#include "BuiltInEffects.h"
#include "BuiltInEffectResources.h"
#include "ReverbPresetParameters.h"
#include <global/settings.h>
#include <log.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

namespace dgk
{
namespace
{
constexpr auto fileName = "effects.json";
// The file the compressor and limiter had before the reverb joined them.
constexpr auto legacyFileName = "dynamics.json";
const muse::Settings::Key REVERB_PRESET("OrchestrionSynthesis",
                                        "REVERB_PRESET");
constexpr ReverbPreset defaultReverbPreset = ReverbPreset::SmallHall;

// The defaults are Audacity's, but for the compressor's look-ahead: 1 ms
// rather than 3, this being played live.
CompressorSettings defaultCompressor()
{
  CompressorSettings settings;
  settings.lookaheadMs = 1;
  return settings;
}

LimiterSettings defaultLimiter() { return LimiterSettings{}; }

QJsonObject toJson(const CompressorSettings &s)
{
  return {{"thresholdDb", s.thresholdDb},
          {"kneeWidthDb", s.kneeWidthDb},
          {"compressionRatio", s.compressionRatio},
          {"lookaheadMs", s.lookaheadMs},
          {"attackMs", s.attackMs},
          {"releaseMs", s.releaseMs},
          {"makeupGainDb", s.makeupGainDb}};
}

QJsonObject toJson(const LimiterSettings &s)
{
  return {{"thresholdDb", s.thresholdDb},
          {"makeupTargetDb", s.makeupTargetDb},
          {"kneeWidthDb", s.kneeWidthDb},
          {"lookaheadMs", s.lookaheadMs},
          {"releaseMs", s.releaseMs}};
}

// A missing or malformed key keeps its default.
void read(const QJsonObject &obj, const char *key, double &value)
{
  if (obj.contains(key) && obj.value(key).isDouble())
    value = obj.value(key).toDouble();
  else
    LOGW() << "effects parameters: no number for '" << key << "', keeping "
           << value;
}

CompressorSettings compressorFromJson(const QJsonObject &obj)
{
  CompressorSettings s = defaultCompressor();
  read(obj, "thresholdDb", s.thresholdDb);
  read(obj, "kneeWidthDb", s.kneeWidthDb);
  read(obj, "compressionRatio", s.compressionRatio);
  read(obj, "lookaheadMs", s.lookaheadMs);
  read(obj, "attackMs", s.attackMs);
  read(obj, "releaseMs", s.releaseMs);
  read(obj, "makeupGainDb", s.makeupGainDb);
  return s;
}

LimiterSettings limiterFromJson(const QJsonObject &obj)
{
  LimiterSettings s = defaultLimiter();
  read(obj, "thresholdDb", s.thresholdDb);
  read(obj, "makeupTargetDb", s.makeupTargetDb);
  read(obj, "kneeWidthDb", s.kneeWidthDb);
  read(obj, "lookaheadMs", s.lookaheadMs);
  read(obj, "releaseMs", s.releaseMs);
  return s;
}

QJsonObject readObject(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return {};
  return QJsonDocument::fromJson(file.readAll()).object();
}

// The reverb's parameters used to be in the file too, before it got presets:
// a file from those days loses them, lest someone edit them in vain.
void dropReverbSections(const QString &path)
{
  QJsonObject root = readObject(path);
  if (!root.contains("reverb") && !root.contains("reverbRanges"))
    return;
  root.remove("reverb");
  root.remove("reverbRanges");
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return;
  file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  LOGI() << "Reverb parameters dropped from " << path.toStdString()
         << " (the reverb has presets now)";
}
} // namespace

BuiltInEffects::BuiltInEffects()
    : m_runtime{
          std::make_shared<ParameterStore<DynamicRangeProcessorSettings>>(
              DynamicRangeProcessorSettings{defaultCompressor()}),
          std::make_shared<ParameterStore<DynamicRangeProcessorSettings>>(
              DynamicRangeProcessorSettings{defaultLimiter()}),
          std::make_shared<ParameterStore<ReverbParameters>>(
              reverbPresetParameters(defaultReverbPreset)),
          {{BuiltInEffect::Compressor, std::make_shared<EffectMeter>()},
           {BuiltInEffect::Limiter, std::make_shared<EffectMeter>()},
           {BuiltInEffect::Reverb, std::make_shared<EffectMeter>()}}}
{
}

void BuiltInEffects::init()
{
  muse::settings()->setDefaultValue(
      REVERB_PRESET, muse::Val{std::string(reverbPresetKey(defaultReverbPreset))});
  m_runtime.reverb->set(reverbPresetParameters(reverbPreset()));

  const QString path = parametersFilePath().toQString();
  if (!QFileInfo::exists(path))
  {
    // The previous file's compressor and limiter carry over.
    const QString legacyPath =
        (globalConfiguration()->userAppDataPath() + "/" + legacyFileName)
            .toQString();
    writeDefaults(path, readObject(legacyPath));
  }
  else
    dropReverbSections(path);
  load(path);
  watch(path);
}

BuiltInEffectRuntime BuiltInEffects::runtime() const { return m_runtime; }

std::shared_ptr<EffectMeter> BuiltInEffects::meter(BuiltInEffect effect) const
{
  return m_runtime.meters.at(effect);
}

muse::io::path_t BuiltInEffects::parametersFilePath() const
{
  return globalConfiguration()->userAppDataPath() + "/" + fileName;
}

ReverbPreset BuiltInEffects::reverbPreset() const
{
  return reverbPresetFromKey(muse::settings()->value(REVERB_PRESET).toString())
      .value_or(defaultReverbPreset);
}

void BuiltInEffects::setReverbPreset(ReverbPreset preset)
{
  if (preset == reverbPreset())
    return;
  muse::settings()->setSharedValue(
      REVERB_PRESET, muse::Val{std::string(reverbPresetKey(preset))});
  m_runtime.reverb->set(reverbPresetParameters(preset));
  LOGI() << "Reverb preset: " << reverbPresetKey(preset);
  m_reverbPresetChanged.notify();
}

muse::async::Notification BuiltInEffects::reverbPresetChanged() const
{
  return m_reverbPresetChanged;
}

void BuiltInEffects::writeDefaults(const QString &path,
                                   const QJsonObject &legacyDynamics) const
{
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    LOGW() << "Cannot write the effects parameters to " << path.toStdString();
    return;
  }
  // The previous file's values over the defaults, so every key is present.
  const auto merged = [&legacyDynamics](const char *section, QJsonObject defaults)
  {
    const QJsonObject legacy = legacyDynamics.value(section).toObject();
    for (const QString &key : legacy.keys())
      if (defaults.contains(key) && legacy.value(key).isDouble())
        defaults.insert(key, legacy.value(key));
    return defaults;
  };
  QJsonObject root;
  root.insert("compressor", merged("compressor", toJson(defaultCompressor())));
  root.insert("limiter", merged("limiter", toJson(defaultLimiter())));
  file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  LOGI() << "Effects parameters written with their defaults: "
         << path.toStdString();
}

void BuiltInEffects::load(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
  {
    LOGW() << "Cannot read the effects parameters from " << path.toStdString();
    return;
  }
  QJsonParseError error;
  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
  if (!doc.isObject())
  {
    // Typically an edit in progress; the previous parameters stay.
    LOGW() << "Effects parameters: " << error.errorString().toStdString()
           << " (" << path.toStdString() << ")";
    return;
  }
  const QJsonObject root = doc.object();
  m_runtime.compressor->set(DynamicRangeProcessorSettings{
      compressorFromJson(root.value("compressor").toObject())});
  m_runtime.limiter->set(DynamicRangeProcessorSettings{
      limiterFromJson(root.value("limiter").toObject())});
  LOGI() << "Effects parameters loaded from " << path.toStdString();
}

void BuiltInEffects::watch(const QString &path)
{
  // Editors usually replace the file rather than rewrite it, which ends the
  // watch on the old inode: the directory is watched too, to pick the new
  // file up.
  m_watcher.addPath(path);
  m_watcher.addPath(QFileInfo(path).absolutePath());
  const auto reload = [this, path]
  {
    if (!QFileInfo::exists(path))
      return;
    if (!m_watcher.files().contains(path))
      m_watcher.addPath(path);
    load(path);
  };
  QObject::connect(&m_watcher, &QFileSystemWatcher::fileChanged, &m_watcher,
                   [reload](const QString &) { reload(); });
  QObject::connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
                   &m_watcher,
                   [this, path, reload](const QString &)
                   {
                     if (!m_watcher.files().contains(path))
                       reload();
                   });
}
} // namespace dgk
