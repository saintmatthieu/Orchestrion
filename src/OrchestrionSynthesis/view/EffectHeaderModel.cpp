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
#include "EffectHeaderModel.h"
#include <vst/ivstplugininstance.h>

#include <algorithm>

namespace dgk
{
EffectHeaderModel::EffectHeaderModel(QObject *parent) : QObject(parent)
{
  effectChain()->chainChanged().onNotify(this, [this] { refresh(); });
}

QString EffectHeaderModel::effectId() const
{
  return QString::fromStdString(m_effectId);
}

void EffectHeaderModel::setEffectId(const QString &effectId)
{
  const std::string id = effectId.toStdString();
  if (id == m_effectId)
    return;
  m_effectId = id;
  emit effectChanged();
  refresh();
}

int EffectHeaderModel::vstInstanceId() const { return m_vstInstanceId; }

void EffectHeaderModel::setVstInstanceId(int instanceId)
{
  if (instanceId == m_vstInstanceId)
    return;
  m_vstInstanceId = instanceId;
  // The instance knows its plugin's resource id, which is the effect's.
  if (const muse::vst::IVstPluginInstancePtr instance =
          vstInstancesRegister()->instanceById(instanceId))
    m_effectId = instance->resourceId();
  emit effectChanged();
  refresh();
}

QString EffectHeaderModel::title() const
{
  const std::vector<EffectDesc> chain = effectChain()->chain();
  const auto it = std::find_if(chain.begin(), chain.end(),
                               [this](const EffectDesc &effect)
                               { return effect.id == m_effectId; });
  return QString::fromStdString(it != chain.end() ? it->name : m_effectId);
}

bool EffectHeaderModel::active() const { return m_active; }

void EffectHeaderModel::toggleActive()
{
  effectChain()->setActive(m_effectId, !m_active);
}

void EffectHeaderModel::refresh()
{
  const bool active = effectChain()->isActive(m_effectId);
  if (active == m_active)
    return;
  m_active = active;
  emit activeChanged();
}
} // namespace dgk
