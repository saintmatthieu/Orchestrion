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
#include "OpenRequestHandler.h"
#include "MuseScoreShell/OrchestrionActionIds.h"

#include <async/async.h>
#include <global/translation.h>
#include <log.h>
#include <project/inotationproject.h>

#include <QFileInfo>
#include <QWindow>
#include <algorithm>
#include <utility>

namespace dgk
{
std::optional<QString> rollsIdOfLink(const QUrl &link)
{
  // orchestrion://rolls/<id>. A query or a fragment, which later versions of
  // the links may have, is ignored.
  if (!link.isValid() || link.scheme() != "orchestrion" ||
      link.host() != "rolls" || !link.userInfo().isEmpty() || link.port() != -1)
    return std::nullopt;
  QString id = link.path();
  if (!id.startsWith('/'))
    return std::nullopt;
  id.remove(0, 1);
  // Some browsers add one.
  if (id.endsWith('/'))
    id.chop(1);
  if (!isRollsId(id))
    return std::nullopt;
  return id;
}

void OpenRequestHandler::open(const QUrl &url)
{
  if (m_ready)
    carryOut(url);
  else
    m_pending.push_back(url);
}

bool OpenRequestHandler::hasPending() const
{
  return std::any_of(m_pending.begin(), m_pending.end(),
                     [](const QUrl &url) { return !url.isEmpty(); });
}

void OpenRequestHandler::setReady()
{
  if (m_ready)
    return;
  m_ready = true;
  // Deferred: the score shelf, which opens the scores of Rolls, is created
  // along with the main window.
  muse::async::Async::call(this,
                           [this]
                           {
                             for (const QUrl &url :
                                  std::exchange(m_pending, {}))
                               carryOut(url);
                           });
}

void OpenRequestHandler::carryOut(const QUrl &url)
{
  bringToFront();
  if (url.isEmpty())
    return;

  const auto project = globalContext()->currentProject();
  const QString openPath = project ? project->path().toQString() : QString{};

  if (url.isLocalFile())
  {
    const QString path = url.toLocalFile();
    if (path == openPath || !mayInterrupt(QFileInfo{path}.completeBaseName()))
      return;
    dispatcher()->dispatch(
        "orchestrion-file-open",
        muse::actions::ActionData::make_arg2<QUrl, QString>(url, QString{}));
    return;
  }

  const std::optional<QString> id = rollsIdOfLink(url);
  if (!id)
  {
    LOGW() << "Not a link Orchestrion opens: " << url.toString();
    return;
  }
  if (rollsLibrary()->localPath(*id) == openPath)
    return;
  if (rollsLibrary()->score(*id))
  {
    openRolls(*id);
    return;
  }
  // Not in the index on this computer: it may have been added since.
  if (!m_subscribed)
  {
    m_subscribed = true;
    rollsLibrary()->indexFetched().onReceive(this, [this](bool reached)
                                             { onIndexFetched(reached); });
  }
  m_waitingId = *id;
  rollsLibrary()->refreshNow();
}

void OpenRequestHandler::onIndexFetched(bool reached)
{
  if (m_waitingId.isEmpty())
    return;
  const QString id = std::exchange(m_waitingId, {});
  if (rollsLibrary()->score(id))
    openRolls(id);
  else if (reached)
    tellFailure(
        muse::qtrc("orchestrion/links", "This score isn’t in the Library"),
        muse::qtrc("orchestrion/links",
                   "It may be too new for this version of Orchestrion: try "
                   "updating it."));
  else
    tellFailure(muse::qtrc("orchestrion/links", "Rolls couldn’t be reached"),
                muse::qtrc("orchestrion/links",
                           "The score can’t be downloaded. Check your internet "
                           "connection, then open the link again."));
}

void OpenRequestHandler::openRolls(const QString &id)
{
  const RollsScore *const score = rollsLibrary()->score(id);
  IF_ASSERT_FAILED(score) { return; }
  if (!mayInterrupt(score->title))
    return;
  dispatcher()->dispatch(actionIds::openRollsScore,
                         muse::actions::ActionData::make_arg1<QString>(id));
}

bool OpenRequestHandler::mayInterrupt(const QString &title) const
{
  bool playing = false;
  if (const IOrchestrionPlayerPtr player = orchestrion()->player())
    playing = player->IsPlaying();
  if (const IOrchestrionSequencerPtr sequencer = orchestrion()->sequencer();
      !playing && sequencer)
  {
    // Someone playing the score themselves.
    const std::optional<PerformedTempo> tempo = sequencer->PerformedTempoNow();
    playing = tempo && !tempo->coasting;
  }
  if (!playing)
    return true;

  using muse::IInteractive;
  constexpr int openButton = int(IInteractive::Button::CustomButton) + 1;
  // "Not now" by default: the keys the performer is pressing mustn't open it.
  const IInteractive::Result result = interactive()->questionSync(
      muse::qtrc("orchestrion/links", "Open “%1”?").arg(title).toStdString(),
      muse::trc("orchestrion/links", "This stops the performance."),
      IInteractive::ButtonDatas{
          IInteractive::ButtonData{IInteractive::Button::Cancel,
                                   muse::trc("orchestrion/links", "Not now"),
                                   true},
          IInteractive::ButtonData{openButton,
                                   muse::trc("orchestrion/links", "Open")}},
      int(IInteractive::Button::Cancel));
  return result.button() == openButton;
}

void OpenRequestHandler::tellFailure(const QString &title, const QString &text)
{
  interactive()->info(title.toStdString(), text.toStdString());
  // Nothing to go back to: the shelf offers the other scores.
  if (!globalContext()->currentProject())
    dispatcher()->dispatch(actionIds::browseScores);
}

void OpenRequestHandler::bringToFront()
{
  if (QWindow *const window = mainWindow()->qWindow())
    window->setWindowStates(window->windowStates() & ~Qt::WindowMinimized);
  mainWindow()->requestShowOnFront();
}
} // namespace dgk
