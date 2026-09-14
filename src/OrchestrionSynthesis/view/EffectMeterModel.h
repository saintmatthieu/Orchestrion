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
#pragma once

#include "IBuiltInEffects.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include <async/asyncable.h>
#include <modularity/ioc.h>

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <memory>

namespace dgk
{
/**
 * The meters of a built-in effect for its window: input and output level in
 * dBFS, gain reduction in dB for the dynamics effects (all with a peak hold
 * that falls back at a readable pace), the clip indicator, and for the reverb
 * its presets.
 */
class EffectMeterModel : public QObject,
                         public dgk::Injectable,
                         public muse::async::Asyncable
{
  Q_OBJECT
  Q_PROPERTY(QString effect READ effect WRITE setEffect NOTIFY effectChanged)
  Q_PROPERTY(QString title READ title NOTIFY effectChanged)
  Q_PROPERTY(QString effectId READ effectId NOTIFY effectChanged)
  Q_PROPERTY(bool hasGainReduction READ hasGainReduction NOTIFY effectChanged)
  Q_PROPERTY(QString parametersFilePath READ parametersFilePath NOTIFY
                 effectChanged)
  Q_PROPERTY(double inputDb READ inputDb NOTIFY metersChanged)
  Q_PROPERTY(double outputDb READ outputDb NOTIFY metersChanged)
  Q_PROPERTY(double gainReductionDb READ gainReductionDb NOTIFY metersChanged)
  Q_PROPERTY(bool clipped READ clipped NOTIFY metersChanged)
  Q_PROPERTY(bool hasPresets READ hasPresets NOTIFY effectChanged)
  //! [{key, name}], in order; \c preset is the current one's key.
  Q_PROPERTY(QVariantList presets READ presets NOTIFY effectChanged)
  Q_PROPERTY(QString preset READ preset WRITE setPreset NOTIFY presetChanged)

  dgk::Inject<IBuiltInEffects> builtInEffects{this};

public:
  explicit EffectMeterModel(QObject *parent = nullptr);

  QString effect() const;
  void setEffect(const QString &effect);
  QString title() const;
  QString effectId() const;
  bool hasGainReduction() const;
  QString parametersFilePath() const;
  double inputDb() const;
  double outputDb() const;
  double gainReductionDb() const;
  bool clipped() const;
  bool hasPresets() const;
  QVariantList presets() const;
  QString preset() const;
  void setPreset(const QString &key);

  Q_INVOKABLE void resetClip();

signals:
  void effectChanged();
  void metersChanged();
  void presetChanged();

private:
  void refresh();

  QString m_effect;
  std::optional<BuiltInEffect> m_kind;
  std::shared_ptr<EffectMeter> m_meter;
  QTimer m_timer;
  double m_inputDb = -90;
  double m_outputDb = -90;
  double m_gainReductionDb = 0;
  bool m_clipped = false;
};
} // namespace dgk
