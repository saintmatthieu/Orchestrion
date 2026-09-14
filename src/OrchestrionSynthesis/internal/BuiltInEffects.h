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

#include "BuiltInEffectRuntime.h"
#include "IBuiltInEffects.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include <global/iglobalconfiguration.h>
#include <modularity/ioc.h>

#include <QFileSystemWatcher>
#include <QJsonObject>
#include <QString>

namespace dgk
{
/**
 * Owns the built-in effects' parameters and meters. The parameters come from
 * a JSON file in the application's data directory, written with the defaults
 * on first use and watched afterwards: saving it in an editor changes the
 * running effects.
 */
class BuiltInEffects : public IBuiltInEffects, public dgk::Injectable
{
  dgk::Inject<muse::IGlobalConfiguration> globalConfiguration{this};

public:
  BuiltInEffects();

  void init();

  /** What the audio engine's resolver needs to create the effects. */
  BuiltInEffectRuntime runtime() const;

  // IBuiltInEffects
private:
  std::shared_ptr<EffectMeter> meter(BuiltInEffect effect) const override;
  muse::io::path_t parametersFilePath() const override;

private:
  void writeDefaults(const QString &path, const QJsonObject &legacyDynamics) const;
  void load(const QString &path);
  void watch(const QString &path);

  const BuiltInEffectRuntime m_runtime;
  QFileSystemWatcher m_watcher;
};
} // namespace dgk
