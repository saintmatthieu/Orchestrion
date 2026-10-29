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

#include "OrchestrionSequencer/IOrchestrion.h"
#include "OrchestrionShell/IOpenRequestHandler.h"
#include "OrchestrionShell/IRollsLibrary.h"

#include <actions/iactionsdispatcher.h>
#include <async/asyncable.h>
#include <context/iglobalcontext.h>
#include <interactive/iinteractive.h>
#include <ui/imainwindow.h>

#include <optional>
#include <vector>

#include "OrchestrionCommon/OrchestrionIoc.h"

namespace dgk
{
/** The id in a link to a score of Rolls, if that is what it is. */
std::optional<QString> rollsIdOfLink(const QUrl &link);

/**
 * IOpenRequestHandler. A link's score is looked up in the index, fetched
 * again if it isn't there (the score may be new), and opened from the score
 * shelf, which shows its download.
 */
class OpenRequestHandler : public IOpenRequestHandler,
                           public dgk::Injectable,
                           public muse::async::Asyncable
{
  dgk::Inject<IRollsLibrary> rollsLibrary{this};
  dgk::Inject<IOrchestrion> orchestrion{this};
  dgk::Inject<muse::actions::IActionsDispatcher> dispatcher{this};
  dgk::Inject<mu::context::IGlobalContext> globalContext{this};
  dgk::Inject<muse::IInteractive> interactive{this};
  dgk::Inject<muse::ui::IMainWindow> mainWindow{this};

public:
  void open(const QUrl &url) override;
  bool hasPending() const override;
  void setReady() override;

private:
  void carryOut(const QUrl &url);
  /** Opens the score of the link, or says why not, once the index has it. */
  void openRolls(const QString &id);
  void onIndexFetched(bool reached);
  /** Whether the user agrees to stop what is playing, if anything is. */
  bool mayInterrupt(const QString &title) const;
  /** Says it, and offers the shelf if there is no score to go back to. */
  void tellFailure(const QString &title, const QString &text);
  void bringToFront();

  bool m_ready = false;
  std::vector<QUrl> m_pending;
  /** The link waiting for the index to be fetched again. */
  QString m_waitingId;
  bool m_subscribed = false;
};
} // namespace dgk
