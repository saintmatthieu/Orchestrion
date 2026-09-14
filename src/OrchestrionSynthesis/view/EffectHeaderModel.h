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

#include "IEffectChain.h"
#include "OrchestrionCommon/OrchestrionIoc.h"
#include <async/asyncable.h>
#include <modularity/ioc.h>
#include <vst/ivstinstancesregister.h>

#include <QObject>

namespace dgk
{
/**
 * The header of an effect's window: which effect of the chain it is, and its
 * bypass state. Identified either by the effect's id (built-in effects) or by
 * the VST plugin instance the window shows.
 */
class EffectHeaderModel : public QObject,
                          public dgk::Injectable,
                          public muse::async::Asyncable
{
  Q_OBJECT
  Q_PROPERTY(QString effectId READ effectId WRITE setEffectId NOTIFY
                 effectChanged)
  Q_PROPERTY(int vstInstanceId READ vstInstanceId WRITE setVstInstanceId NOTIFY
                 effectChanged)
  Q_PROPERTY(QString title READ title NOTIFY effectChanged)
  Q_PROPERTY(bool active READ active NOTIFY activeChanged)

  dgk::Inject<IEffectChain> effectChain{this};
  dgk::Inject<muse::vst::IVstInstancesRegister> vstInstancesRegister{this};

public:
  explicit EffectHeaderModel(QObject *parent = nullptr);

  QString effectId() const;
  void setEffectId(const QString &effectId);
  int vstInstanceId() const;
  void setVstInstanceId(int instanceId);
  QString title() const;
  bool active() const;

  Q_INVOKABLE void toggleActive();

signals:
  void effectChanged();
  void activeChanged();

private:
  void refresh();

  std::string m_effectId;
  int m_vstInstanceId = -1;
  bool m_active = true;
};
} // namespace dgk
