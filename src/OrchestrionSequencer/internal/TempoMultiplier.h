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

#include "ITempoMultiplier.h"
#include "OrchestrionCommon/OrchestrionIoc.h"

#include <async/asyncable.h>
#include <context/iglobalcontext.h>

namespace dgk
{
/**
 * The tempo multiplier, kept in a meta tag of the score (as the staff modes
 * are, see StaffModes) and changed through the score's undo stack. What the
 * tag says is handed on to the score's tempo timeline, which every tempo
 * Orchestrion plays at is read from.
 */
class TempoMultiplier : public ITempoMultiplier,
                        public muse::async::Asyncable,
                        public dgk::Injectable
{
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};

public:
  void init();

  double multiplier() const override;
  void setMultiplier(double) override;
  muse::async::Notification multiplierChanged() const override;

private:
  /** What the score says: 1 if nothing. */
  double savedMultiplier() const;
  /** Hands the saved multiplier on to the score, notifying if it changed. */
  void checkChanged();

  double m_multiplier = 1.0;
  muse::async::Notification m_multiplierChanged;
};
} // namespace dgk
