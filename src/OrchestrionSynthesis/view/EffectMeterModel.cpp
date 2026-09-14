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
#include "EffectMeterModel.h"
#include "ReverbPresets.h"
#include "internal/BuiltInEffectResources.h"

#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace dgk
{
namespace
{
constexpr int refreshIntervalMs = 33;
constexpr double silenceDb = -90;
// Peak hold: a level falls back this much per refresh.
constexpr double levelFallDbPerTick = 1.5;
constexpr double reductionFallDbPerTick = 1.0;

double toDb(float peak)
{
  return peak > 0.f ? 20. * std::log10(peak) : silenceDb;
}
} // namespace

EffectMeterModel::EffectMeterModel(QObject *parent) : QObject(parent)
{
  m_timer.setInterval(refreshIntervalMs);
  connect(&m_timer, &QTimer::timeout, this, [this] { refresh(); });
  builtInEffects()->reverbPresetChanged().onNotify(this, [this]
                                                   { emit presetChanged(); });
}

QString EffectMeterModel::effect() const { return m_effect; }

void EffectMeterModel::setEffect(const QString &effect)
{
  if (effect == m_effect)
    return;
  m_effect = effect;
  m_kind = builtInEffectFromKey(effect.toStdString());
  m_meter = m_kind ? builtInEffects()->meter(*m_kind) : nullptr;
  if (m_meter)
    m_timer.start();
  else
    m_timer.stop();
  emit effectChanged();
  refresh();
}

QString EffectMeterModel::title() const
{
  return m_kind ? QString::fromStdString(builtInEffectName(*m_kind)) : m_effect;
}

QString EffectMeterModel::effectId() const
{
  return m_kind ? QString::fromStdString(builtInEffectId(*m_kind)) : QString();
}

bool EffectMeterModel::hasGainReduction() const
{
  return m_kind && *m_kind != BuiltInEffect::Reverb;
}

QString EffectMeterModel::parametersFilePath() const
{
  return builtInEffects()->parametersFilePath().toQString();
}

double EffectMeterModel::inputDb() const { return m_inputDb; }
double EffectMeterModel::outputDb() const { return m_outputDb; }
double EffectMeterModel::gainReductionDb() const { return m_gainReductionDb; }
bool EffectMeterModel::clipped() const { return m_clipped; }

bool EffectMeterModel::hasPresets() const
{
  return m_kind && *m_kind == BuiltInEffect::Reverb;
}

QVariantList EffectMeterModel::presets() const
{
  QVariantList result;
  if (!hasPresets())
    return result;
  for (const auto preset : allReverbPresets)
    result.append(QVariantMap{
        {"key", QString::fromUtf8(reverbPresetKey(preset))},
        {"name", QString::fromStdString(reverbPresetName(preset))}});
  return result;
}

QString EffectMeterModel::preset() const
{
  return hasPresets()
             ? QString::fromUtf8(reverbPresetKey(builtInEffects()->reverbPreset()))
             : QString();
}

void EffectMeterModel::setPreset(const QString &key)
{
  if (const std::optional<ReverbPreset> preset =
          reverbPresetFromKey(key.toStdString()))
    builtInEffects()->setReverbPreset(*preset);
}

void EffectMeterModel::resetClip()
{
  if (m_meter)
    m_meter->clipped.store(false, std::memory_order_relaxed);
  refresh();
}

void EffectMeterModel::refresh()
{
  if (!m_meter)
    return;
  const auto hold = [](double shown, double now, double fall)
  { return std::max(now, shown - fall); };
  m_inputDb = hold(m_inputDb,
                   toDb(m_meter->inputPeak.load(std::memory_order_relaxed)),
                   levelFallDbPerTick);
  m_outputDb = hold(m_outputDb,
                    toDb(m_meter->outputPeak.load(std::memory_order_relaxed)),
                    levelFallDbPerTick);
  m_gainReductionDb =
      hold(m_gainReductionDb,
           m_meter->gainReductionDb.load(std::memory_order_relaxed),
           reductionFallDbPerTick);
  m_clipped = m_meter->clipped.load(std::memory_order_relaxed);
  emit metersChanged();
}
} // namespace dgk
