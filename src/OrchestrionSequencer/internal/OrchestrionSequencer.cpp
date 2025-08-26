/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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
#include "OrchestrionSequencer.h"
#include "IChord.h"
#include <algorithm>
#include <engraving/dom/note.h>
#include <iterator>
#include <notation/inotationmidiinput.h>
#include <numeric>
#include <optional>

#ifdef OS_IS_WIN
#include <windows.h>
#endif

namespace dgk
{
namespace
{
auto MakeAllVoices(const OrchestrionSequencer::HandVoices &rightHand,
                   const OrchestrionSequencer::HandVoices &leftHand)
{
  std::vector<const VoiceSequencer *> allVoices;
  allVoices.reserve(rightHand.size() + leftHand.size());
  std::transform(rightHand.begin(), rightHand.end(),
                 std::back_inserter(allVoices),
                 [](const auto &voice) { return voice.get(); });
  std::transform(leftHand.begin(), leftHand.end(),
                 std::back_inserter(allVoices),
                 [](const auto &voice) { return voice.get(); });
  return allVoices;
}

auto GetFinalTick(const std::vector<const VoiceSequencer *> &voices)
{
  return voices.empty()
             ? dgk::Tick{0, 0}
             : (*std::max_element(
                    voices.begin(), voices.end(),
                    [](const VoiceSequencer *a, const VoiceSequencer *b) -> bool
                    { return a->GetFinalTick() < b->GetFinalTick(); }))
                   ->GetFinalTick();
}
} // namespace

template <typename EventType>
std::thread OrchestrionSequencer::MakeThread(OrchestrionSequencer &self,
                                             ThreadMembers<EventType> &m,
                                             std::function<void(EventType)> cb)
{
  return std::thread{
      [&, cb = cb]
      {
        while (true)
        {
          std::vector<QueueEntry<EventType>> entries;
          {
            std::unique_lock lock{m.mutex};
            m.cv.wait(lock,
                      [&] { return !m.queue.empty() || self.m_finished; });
            if (self.m_finished)
              return;
            while (!m.queue.empty())
            {
              entries.push_back(m.queue.front());
              m.queue.pop();
            }
          }
          for (auto &entry : entries)
          {
            if (entry.time.has_value())
              std::this_thread::sleep_until(*entry.time);
            cb(std::move(entry.event));
          }
        }
      }};
}

OrchestrionSequencer::OrchestrionSequencer(InstrumentIndex instrument,
                                           HandVoices rightHand,
                                           HandVoices leftHand,
                                           PedalSequence pedalSequence)
    : m_instrument{std::move(instrument)},
      m_rightHand{std::move(rightHand), std::nullopt},
      m_leftHand{std::move(leftHand), std::nullopt},
      m_allVoices{MakeAllVoices(m_rightHand.voices, m_leftHand.voices)},
      m_finalTick{GetFinalTick(m_allVoices)},
      m_pedalSequence{std::move(pedalSequence)},
      m_pedalSequenceIt{m_pedalSequence.begin()},
      m_pedalThread{MakeThread<PedalEvent>(*this, m_pedalThreadMembers,
                                           [this](PedalEvent event)
                                           { m_outputEvent.send(event); })},
      m_noteThread{MakeThread<NoteEvent>(
          *this, m_noteThreadMembers,
          [this](NoteEvent event) { m_outputEvent.send(NoteEvents{event}); })}
{
#ifdef OS_IS_WIN
  SetThreadPriority(m_noteThread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  dispatcher()->reg(this, "nav-first-control", [this] { GoToTick(0); });
  interactionProcessor()->itemClicked().onReceive(
      this, [this](const mu::engraving::EngravingItem *item)
      { GoToTick(item->tick().ticks()); });
  GoToTick(0);
}

namespace
{
void AppendNoteoffs(dgk::NoteEvents &output, const std::vector<int> &noteoffs,
                    TrackIndex track, const std::vector<int> &noteons)
{
  if (noteons.empty())
  {
    output.reserve(output.size() + noteoffs.size());
    std::transform(
        noteoffs.begin(), noteoffs.end(), std::back_inserter(output),
        [&](int note)
        { return NoteEvent{NoteEventType::noteOff, track, note, 0.f}; });
  }
  else
    // Only append noteoffs that aren't in `noteons`.
    for (const auto noteoff : noteoffs)
      if (std::find(noteons.begin(), noteons.end(), noteoff) == noteons.end())
        output.push_back(
            NoteEvent{NoteEventType::noteOff, track, noteoff, 0.f});
}

void AppendNoteons(dgk::NoteEvents &output, const std::vector<int> &noteons,
                   TrackIndex track, float velocity)
{
  output.reserve(output.size() + noteons.size());
  std::transform(
      noteons.begin(), noteons.end(), std::back_inserter(output), [&](int note)
      { return NoteEvent{NoteEventType::noteOn, track, note, velocity}; });
}

void UniteStaffTransitions(std::map<TrackIndex, ChordTransition> &transitions)
{
  // Use the time where transitions aren't yet all mingled in one map to apply
  // the per-staff algorithm.
  // For a given staff, there must remain at most one future chord, and it must
  // be that of the earliest.
  std::vector<TrackIndex> futureChordTracks;
  for (const auto &[track, transition] : transitions)
    if (GetFutureChord(transition))
      futureChordTracks.push_back(track);
  if (futureChordTracks.size() < 2)
    return;
  Tick::value_type earliestTick = std::numeric_limits<Tick::value_type>::max();
  for (const auto &track : futureChordTracks)
  {
    const auto tick =
        GetFutureChord(transitions.at(track))->GetBeginTick().withRepeats;
    if (tick < earliestTick)
      earliestTick = tick;
  }
  // All entries with futures that aren't the earliest see their futures
  // removed.
  for (const auto &track : futureChordTracks)
  {
    const auto transition = transitions.at(track);
    const auto tick = GetFutureChord(transition)->GetBeginTick().withRepeats;
    if (tick <= earliestTick)
      continue;
    if (Get<FutureChord>(transition))
      // Nothing left if we remove the future.
      transitions.erase(track);
    else
      transitions.emplace(
          track,
          PastChord{Get<PastChordAndFutureChord>(transition)->pastChord});
  }
}
} // namespace

void OrchestrionSequencer::SendTransitions(
    std::map<TrackIndex, ChordTransition> transitions,
    std::optional<float> velocity, bool isLeftHand)
{
  if (transitions.empty())
    return;

  for (auto &[track, transition] : transitions)
  {
    std::vector<NoteEvent> voiceOutput;
    const auto past = GetPastChord(transition);
    const auto present = GetPresentChord(transition);
    std::vector<int> noteons;
    if (present)
    {
      if (!velocity.has_value())
      {
        if (present->GetVelocity() > 0.f)
          velocity = present->GetVelocity();
        else
          velocity = isLeftHand ? 0.3f : 0.5f;
      }
      else
        present->SetVelocity(*velocity);

      noteons = present->GetPitches();
    }

    if (past)
      AppendNoteoffs(voiceOutput, past->GetPitches(), track, noteons);
    if (present)
      AppendNoteons(voiceOutput, present->GetPitches(), track, *velocity);

    if (!voiceOutput.empty())
      PostNoteEvents(voiceOutput);
  }

  m_transitions.val.clear();
  for (const auto &[track, transition] : transitions)
    m_transitions.val.emplace(track, transition);
  m_transitions.ch.send(transitions);
}

muse::async::Channel<std::map<TrackIndex, ChordTransition>>
OrchestrionSequencer::ChordTransitions() const
{
  return m_transitions.ch;
}

muse::async::Channel<EventVariant> OrchestrionSequencer::OutputEvent() const
{
  return m_outputEvent;
}

std::vector<TrackIndex> OrchestrionSequencer::GetAllVoices() const
{
  std::vector<TrackIndex> result;
  result.reserve(m_allVoices.size());
  std::transform(m_allVoices.begin(), m_allVoices.end(),
                 std::back_inserter(result),
                 [](const VoiceSequencer *voice) { return voice->track; });
  return result;
}

muse::async::Notification OrchestrionSequencer::AboutToJumpPosition() const
{
  return m_aboutToJumpPosition;
}

OrchestrionSequencer::~OrchestrionSequencer()
{
  if (m_pedalDown)
  {
    PostPedalEvent(PedalEvent{m_instrument, false});
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
  }
  m_finished = true;
  m_pedalThreadMembers.cv.notify_one();
  m_noteThreadMembers.cv.notify_one();
  m_pedalThread.join();
  m_noteThread.join();
}

namespace
{
auto GetCursorTick(const OrchestrionSequencer::HandVoices &hand,
                   NoteEventType event)
{
  std::vector<std::optional<dgk::Tick>> ticks(hand.size());
  std::transform(hand.begin(), hand.end(), ticks.begin(),
                 [event](const std::unique_ptr<VoiceSequencer> &voice)
                 {
                   static const std::optional<Tick> upperLimit{};
                   return voice->GetNextMatchingTick(event, upperLimit);
                 });
  std::optional<dgk::Tick> result;
  for (const auto &tick : ticks)
    if (tick.has_value())
      result = result.has_value() ? std::min(*result, *tick) : *tick;
  return result;
}
} // namespace

void OrchestrionSequencer::OnInputEvent(NoteEventType type, int pitch,
                                        std::optional<float> velocity)
{
  const auto loopEnabled = globalContext()
                               ->currentMasterNotation()
                               ->playback()
                               ->loopBoundaries()
                               .enabled;
  OnInputEventRecursive(type, pitch, std::move(velocity), loopEnabled);

  // Make sure to release the pedal if we've reached the end of this part.
  if (m_pedalDown && type == NoteEventType::noteOff &&
      !GetCursorTick(m_rightHand.voices, NoteEventType::noteOff) &&
      !GetCursorTick(m_leftHand.voices, NoteEventType::noteOff))
    PostPedalEvent(PedalEvent{m_instrument, false});
}

std::map<TrackIndex, ChordTransition>
OrchestrionSequencer::PrepareStaffransitions(
    const HandVoices &voices,
    std::function<std::optional<ChordTransition>(VoiceSequencer &)> op)
{
  std::map<TrackIndex, ChordTransition> transitions;
  for (const auto &voiceSequencer : voices)
    if (const auto transition = op(*voiceSequencer))
      transitions.emplace(voiceSequencer->track, *transition);
  UniteStaffTransitions(transitions);
  return transitions;
}

void OrchestrionSequencer::OnInputEventRecursive(NoteEventType type, int pitch,
                                                 std::optional<float> velocity,
                                                 bool loop)
{

  const bool isLeftHand = pitch < 60 && !m_leftHand.voices.empty();
  auto &hand = isLeftHand ? m_leftHand : m_rightHand;
  if (type == NoteEventType::noteOn)
    hand.pressedKey = pitch;
  else if (hand.pressedKey == pitch)
    hand.pressedKey.reset();
  else
    return;

  const auto cursorTick =
      GetCursorTick(hand.voices, type).value_or(m_finalTick);

  const auto &loopBoundaries =
      globalContext()->currentMasterNotation()->playback()->loopBoundaries();

  if (loop && type == NoteEventType::noteOn && loopBoundaries.loopOutTick > 0 &&
      cursorTick.withoutRepeats >= loopBoundaries.loopOutTick)
  {
    GoToTick(loopBoundaries.loopInTick);
    return OnInputEventRecursive(type, pitch, std::move(velocity), false);
  }

  const std::map<TrackIndex, ChordTransition> transitions =
      PrepareStaffransitions(hand.voices,
                             [&](VoiceSequencer &voice) { //
                               return voice.OnInputEvent(type, cursorTick);
                             });

  Finally maybeRewind{[this, doRewind = transitions.empty()]
                      {
                        if (doRewind)
                          GoToTick(0);
                      }};

  SendTransitions(transitions, std::move(velocity), isLeftHand);

  // For the pedal we wait on the slowest of both hands.
  const auto leastPedalTick = std::accumulate(
      m_allVoices.begin(), m_allVoices.end(), std::optional<dgk::Tick>{},
      [&](const auto &acc, const VoiceSequencer *voice)
      {
        const auto tick = voice->GetTickForPedal();
        return tick.has_value()
                   ? std::make_optional(acc.has_value() ? std::min(*acc, *tick)
                                                        : *tick)
                   : acc;
      });

  const auto newPedalSequenceIt =
      leastPedalTick.has_value()
          ? std::find_if(m_pedalSequenceIt, m_pedalSequence.end(),
                         [&](const auto &item)
                         { return item.tick > leastPedalTick->withRepeats; })
          : m_pedalSequence.end();
  if (newPedalSequenceIt > m_pedalSequenceIt)
  {
    if (type == NoteEventType::noteOff)
    {
      // Just a release.
      PostPedalEvent(PedalEvent{m_instrument, false});
    }
    else
    {
      const auto &item = *(newPedalSequenceIt - 1);
      PostPedalEvent(PedalEvent{m_instrument, item.down});
      m_pedalSequenceIt = newPedalSequenceIt;
    }
  }
}

void OrchestrionSequencer::GoToTick(int tick)
{
  m_aboutToJumpPosition.notify();
  {
    std::map<TrackIndex, ChordTransition> transitions;
    for (auto voices : {&m_rightHand.voices, &m_leftHand.voices})
      transitions.merge(
          PrepareStaffransitions(*voices, [&](VoiceSequencer &voice)
                                 { return voice.GoToTick(tick); }));
    SendTransitions(std::move(transitions));
  }
  m_outputEvent.send(PedalEvent{m_instrument, false});
  m_pedalSequenceIt = std::lower_bound(
      m_pedalSequence.begin(), m_pedalSequence.end(), tick,
      [](const auto &item, int tick) { return item.tick < tick; });
}

const std::map<TrackIndex, ChordTransition> &
OrchestrionSequencer::GetCurrentTransitions() const
{
  return m_transitions.val;
}

void OrchestrionSequencer::PostPedalEvent(PedalEvent event)
{
  OptTimePoint actionTime;
  if (event.on)
  {
    using namespace std::chrono_literals;
    // Delay a bit actioning the pedal so that, if it were just released, the
    // dampers have time to dampen the notes.
    actionTime = std::chrono::steady_clock::now() + 100ms;
  }

  auto &m = m_pedalThreadMembers;
  {
    std::unique_lock lock{m.mutex};
    if (event.on && m_pedalDown)
      // Always insert a pedal off event between two pedal on events.
      m.queue.emplace(QueueEntry<PedalEvent>{std::nullopt,
                                             PedalEvent{m_instrument, false}});
    m.queue.emplace(QueueEntry<PedalEvent>{actionTime, std::move(event)});
  }

  m.cv.notify_one();
  m_pedalDown = event.on;
}

void OrchestrionSequencer::PostNoteEvents(NoteEvents events)
{

  using namespace std::chrono;

  const auto numNoteons =
      std::count_if(events.begin(), events.end(), [](const auto &event)
                    { return event.type == NoteEventType::noteOn; });
  if (numNoteons < 2)
  {
    m_outputEvent.send(std::move(events));
    return;
  }

  // Do not add unnecessary delay.
  std::vector<int> delays(events.size());
  std::generate(delays.begin(), delays.end(),
                [&] { return m_delayDist(m_rng); });
  // We sort the delays, and use the sorted order to shuffle the events.
  const auto unsorted = delays;
  std::sort(delays.begin(), delays.end());
  NoteEvents shuffledEvents;
  shuffledEvents.reserve(events.size());
  for (auto i = 0u; i < events.size(); ++i)
  {
    const auto it = std::find(unsorted.begin(), unsorted.end(), delays[i]);
    const auto &e = events[it - unsorted.begin()];
    shuffledEvents.emplace_back(e.type, e.track, e.pitch, e.velocity);
  }
  std::transform(delays.begin(), delays.end(), delays.begin(),
                 [&](int delay) { return delay - delays[0]; });

  std::vector<QueueEntry<NoteEvent>> entries;
  entries.reserve(shuffledEvents.size());
  const auto now = steady_clock::now();
  for (auto i = 0u; i < shuffledEvents.size(); ++i)
  {
    const auto &event = shuffledEvents[i];
    const auto velocity =
        std::clamp(event.velocity * m_velocityDist(m_rng) / 100, 0.f, 1.f);
    entries.emplace_back(QueueEntry<NoteEvent>{
        now + microseconds{delays[i]},
        NoteEvent{event.type, event.track, event.pitch, velocity}});
  }

  auto &m = m_noteThreadMembers;
  {
    std::unique_lock lock{m.mutex};
    for (auto &entry : entries)
      m.queue.push(std::move(entry));
  }

  m.cv.notify_one();
}

} // namespace dgk
