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

#include <ui/iuicontextresolver.h>

namespace dgk
{
/**
 * The UI actions register enables an action only once a context resolver says
 * its context is the current one. MuseScore's was in its context module, which
 * is gone (its UI actions are turning into commands), and without one every
 * action, the menus' included, stays disabled. Orchestrion's actions are for
 * any context: this one matches every context, and leaves enabling to each
 * action's module.
 */
class OrchestrionUiContextResolver : public muse::ui::IUiContextResolver
{
public:
  const muse::ui::UiContext &currentUiContext() const override;
  muse::async::Notification currentUiContextChanged() const override;

  bool match(const muse::ui::UiContext &currentCtx,
             const muse::ui::UiContext &actCtx) const override;
  bool matchWithCurrent(const muse::ui::UiContext &ctx) const override;

  bool isShortcutContextAllowed(const std::string &scContext) const override;

private:
  muse::async::Notification m_currentUiContextChanged;
};
} // namespace dgk
