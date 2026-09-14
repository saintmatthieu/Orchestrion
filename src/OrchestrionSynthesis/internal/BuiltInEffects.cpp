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
// An earlier name of the file; the values in it carry over.
constexpr auto legacyFileName = "dynamics.json";

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
} // namespace

BuiltInEffects::BuiltInEffects()
    : m_runtime{
          std::make_shared<ParameterStore<DynamicRangeProcessorSettings>>(
              DynamicRangeProcessorSettings{defaultCompressor()}),
          std::make_shared<ParameterStore<DynamicRangeProcessorSettings>>(
              DynamicRangeProcessorSettings{defaultLimiter()}),
          {{BuiltInEffect::Compressor, std::make_shared<EffectMeter>()},
           {BuiltInEffect::Limiter, std::make_shared<EffectMeter>()}}}
{
}

void BuiltInEffects::init()
{
  const QString path = parametersFilePath().toQString();
  if (!QFileInfo::exists(path))
  {
    // The previous file's compressor and limiter carry over.
    const QString legacyPath =
        (globalConfiguration()->userAppDataPath() + "/" + legacyFileName)
            .toQString();
    writeDefaults(path, readObject(legacyPath));
  }
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
