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

#include "async/asyncable.h"
#include "ui/inavigationcontroller.h"
#include <QObject>

#include "OrchestrionCommon/OrchestrionIoc.h"

namespace dgk
{
/**
 * Keyboard navigation (Tab, then the arrows) for the controls that fade out
 * over the score: they show while it moves between them, and their fading
 * out ends it, handing Space and the arrows back to Orchestrion (see
 * OrchestrionShortcutsResolver).
 */
class KeyboardNavigationModel : public QObject,
                                public dgk::Injectable,
                                public muse::async::Asyncable
{
  Q_OBJECT

  dgk::Inject<muse::ui::INavigationController> navigationController{this};

public:
  explicit KeyboardNavigationModel(QObject *parent = nullptr);

  Q_INVOKABLE void load();

  /**
   * Ends keyboard navigation, if on.
   */
  Q_INVOKABLE void release();

signals:
  /**
   * A control got the keyboard focus.
   */
  void navigated();
};
} // namespace dgk
