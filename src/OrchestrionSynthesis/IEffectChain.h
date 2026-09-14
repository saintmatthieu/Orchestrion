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

#include <async/notification.h>
#include <modularity/imoduleinterface.h>
#include <string>
#include <vector>

namespace dgk
{
/**
 * An effect: a VST3 effect plugin known to the application.
 */
struct EffectDesc
{
  std::string id;     // the plugin's audio resource id
  std::string name;   // what to show the user
  std::string vendor; // the plugin's maker, to group long lists by
  bool builtIn = false; // Orchestrion's own (see IBuiltInEffects), not a plugin
  bool active = true;   // false: in the chain but bypassed
};

/**
 * The master effect chain: the effects applied, in order, to the mix of all
 * tracks. Kept across sessions, plugin state included.
 */
class IEffectChain : MODULE_GLOBAL_EXPORT_INTERFACE
{
  INTERFACE_ID(IEffectChain);

public:
  virtual ~IEffectChain() = default;

  /** The effects the user can add to the chain. */
  virtual std::vector<EffectDesc> availableEffects() const = 0;
  virtual muse::async::Notification availableEffectsChanged() const = 0;

  /** The effects in the chain, in processing order. */
  virtual std::vector<EffectDesc> chain() const = 0;
  virtual muse::async::Notification chainChanged() const = 0;
  virtual bool contains(const std::string &effectId) const = 0;

  /**
   * Appends the effect to the chain (no-op if it is already there) and opens
   * its editor once the engine has created it.
   */
  virtual void addEffect(const std::string &effectId) = 0;
  virtual void removeEffect(const std::string &effectId) = 0;

  /** Bypass: an inactive effect stays in the chain but does not process. */
  virtual bool isActive(const std::string &effectId) const = 0;
  virtual void setActive(const std::string &effectId, bool active) = 0;

  /** Opens the plugin's own editor window for an effect of the chain. */
  virtual void openEditor(const std::string &effectId) = 0;
};
} // namespace dgk
