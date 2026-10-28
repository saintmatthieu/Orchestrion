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
#include "ScoreShelfHintModel.h"

#include <global/settings.h>

#include <algorithm>
#include <chrono>

namespace dgk
{
namespace
{
// A moment to take in the end of the piece before the hint comes up.
constexpr auto delayAfterEnd = std::chrono::seconds{3};

const muse::Settings::Key
    SCORE_SHELF_HINT_DISMISSED("OrchestrionShell",
                               "SCORE_SHELF_HINT_DISMISSED");
} // namespace

ScoreShelfHintModel::ScoreShelfHintModel(QObject *parent) : QObject(parent)
{
  m_delay.setSingleShot(true);
  m_delay.setInterval(delayAfterEnd);
  connect(&m_delay, &QTimer::timeout, this, [this] { setShown(true); });
}

void ScoreShelfHintModel::load()
{
  muse::settings()->setDefaultValue(SCORE_SHELF_HINT_DISMISSED,
                                    muse::Val{false});
  m_dismissed = muse::settings()->value(SCORE_SHELF_HINT_DISMISSED).toBool();
  if (m_dismissed)
    return;
  orchestrion()->sequencerChanged().onNotify(this,
                                             [this] { followSequencer(); });
  followSequencer();
}

void ScoreShelfHintModel::followSequencer()
{
  // Another score: the end reached was that of the previous one.
  m_delay.stop();
  setShown(false);
  m_performerStruck = false;
  const auto sequencer = orchestrion()->sequencer();
  if (!sequencer)
    return;
  sequencer->HandNoteEvents().onReceive(
      this,
      [this](const AutoPlayEvent &event)
      {
        if (event.type != NoteEventType::noteOn)
          return;
        // Whose the notes are is told at the strike: the automatic player
        // ("Show me!", a replay) may have stopped by the time its last
        // release gets here.
        m_performerStruck = !orchestrion()->player()->IsPlaying();
        // Playing on: out of the way, till the next end.
        m_delay.stop();
        setShown(false);
      },
      muse::async::Asyncable::Mode::SetReplace);
  // The end, as for the final score: the last notes released, nothing is
  // sounding and nothing is to come on any voice.
  sequencer->ChordTransitions().onReceive(
      this,
      [this](const std::map<TrackIndex, ChordTransition> &)
      {
        if (m_dismissed || !m_performerStruck || m_shown || m_delay.isActive())
          return;
        const auto sequencer = orchestrion()->sequencer();
        if (!sequencer)
          return;
        const auto &current = sequencer->GetCurrentTransitions();
        const bool atEnd = !current.empty() &&
                           std::all_of(current.begin(), current.end(),
                                       [](const auto &entry) {
                                         return !GetFutureChord(entry.second) &&
                                                !GetPresentThing(entry.second);
                                       });
        if (atEnd)
          m_delay.start();
      },
      muse::async::Asyncable::Mode::SetReplace);
}

void ScoreShelfHintModel::dismiss()
{
  m_delay.stop();
  setShown(false);
  if (m_dismissed)
    return;
  m_dismissed = true;
  muse::settings()->setSharedValue(SCORE_SHELF_HINT_DISMISSED, muse::Val{true});
}

bool ScoreShelfHintModel::shown() const { return m_shown; }

void ScoreShelfHintModel::setShown(bool shown)
{
  if (shown == m_shown)
    return;
  m_shown = shown;
  emit shownChanged();
}
} // namespace dgk
