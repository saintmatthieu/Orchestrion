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
#include "OrnamentSchedule.h"
#include "ScoreEvents.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <engraving/dom/note.h>
#include <iterator>
#include <limits>
#include <notation/imasternotation.h>
#include <notation/inotationmidiinput.h>
#include <notation/inotationplayback.h>
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

//! When the right hand has several voices, returns its top (melody) voice —
//! the one with the lowest voice index, MuseScore's stems-up convention.
//! Returns nullopt for a single-voice right hand (nothing to bring out).
std::optional<TrackIndex>
GetUpperRightHandVoice(const OrchestrionSequencer::HandVoices &rightHand)
{
  if (rightHand.size() < 2)
    return std::nullopt;
  const VoiceSequencer *upper = nullptr;
  for (const auto &voice : rightHand)
    if (!upper || voice->track.voiceIndex() < upper->track.voiceIndex())
      upper = voice.get();
  return upper ? std::make_optional(upper->track) : std::nullopt;
}

constexpr double ticksPerQuarter = 480.0;

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
        std::unique_lock lock{m.mutex};
        while (true)
        {
          m.cv.wait(lock, [&] { return !m.queue.empty() || self.m_finished; });
          if (self.m_finished)
            return;
          // An entry stays in the queue until it falls due, so that it can be
          // superseded, and entries queued behind it wait along.
          if (const auto due = m.queue.front().time)
          {
            m.cv.wait_until(lock, *due,
                            [&] {
                              return self.m_finished || m.queue.empty() ||
                                     m.queue.front().time != due;
                            });
            if (self.m_finished)
              return;
            if (m.queue.empty() || m.queue.front().time != due)
              // The queue changed under us; see what's there now.
              continue;
          }
          auto entry = std::move(m.queue.front());
          m.queue.pop_front();
          lock.unlock();
          cb(std::move(entry.event));
          lock.lock();
        }
      }};
}

namespace
{
std::pair<int, int> PitchRange(const std::vector<ScoreEvent> &events)
{
  std::pair<int, int> range{128, -1};
  for (const ScoreEvent &event : events)
    for (const int pitch : event.pitches)
      range = {std::min(range.first, pitch), std::max(range.second, pitch)};
  return range;
}

/** The events of several staves, as one: the onsets at a tick merged. */
std::vector<ScoreEvent>
Merge(const std::vector<const std::vector<ScoreEvent> *> &staves)
{
  std::map<int, ScoreEvent> merged;
  for (const auto *events : staves)
    for (const ScoreEvent &event : *events)
    {
      auto [it, inserted] = merged.try_emplace(event.tick, event);
      if (inserted)
        continue;
      ScoreEvent &into = it->second;
      const auto add = [](std::vector<int> &into, const std::vector<int> &from)
      {
        into.insert(into.end(), from.begin(), from.end());
        std::sort(into.begin(), into.end());
        into.erase(std::unique(into.begin(), into.end()), into.end());
      };
      add(into.pitches, event.pitches);
      add(into.trillPitches, event.trillPitches);
      into.trillEnd = std::max(into.trillEnd, event.trillEnd);
      into.rolled |= event.rolled;
    }
  std::vector<ScoreEvent> events;
  events.reserve(merged.size());
  for (auto &[tick, event] : merged)
    events.push_back(std::move(event));
  // A staff's trill over the other's onsets.
  SpreadTrills(events);
  return events;
}
} // namespace

OrchestrionSequencer::OrchestrionSequencer(
    InstrumentIndex instrument, HandVoices rightHand, HandVoices leftHand,
    PedalSequence pedalSequence,
    std::array<std::vector<ScoreEvent>, 2> staffEvents,
    std::array<std::vector<ScoreNote>, 2> staffNotes, double nominalTicksPerMs)
    : m_instrument{std::move(instrument)},
      m_rightHand{std::move(rightHand), std::nullopt},
      m_leftHand{std::move(leftHand), std::nullopt},
      m_rightHandUpperVoice{GetUpperRightHandVoice(m_rightHand.voices)},
      m_allVoices{MakeAllVoices(m_rightHand.voices, m_leftHand.voices)},
      m_finalTick{GetFinalTick(m_allVoices)},
      m_pedalSequence{std::move(pedalSequence)},
      m_pedalThread{MakeThread<EventVariant>(*this, m_pedalThreadMembers,
                                             [this](EventVariant event)
                                             { m_outputEvent.send(event); })},
      m_noteThread{MakeThread<NoteEvent>(
          *this, m_noteThreadMembers,
          [this](NoteEvent event) { m_outputEvent.send(NoteEvents{event}); })},
      m_ornamentThread{MakeThread<OrnamentStepDue>(
          *this, m_ornamentThreadMembers,
          [this](OrnamentStepDue due) { OnOrnamentStepDue(due); })},
      m_notation{projectSession()->masterNotation().get()},
      m_modes{Mode(false), Mode(true)}, m_staffEvents{std::move(staffEvents)},
      m_staffNotes{std::move(staffNotes)},
      m_staffRanges{PitchRange(m_staffEvents[0]), PitchRange(m_staffEvents[1])},
      m_nominalTicksPerMs{nominalTicksPerMs},
      m_follower{[this](const FollowerThread::Followed &followed)
                 { OnFollowed(followed); }}
{
#ifdef OS_IS_WIN
  SetThreadPriority(m_noteThread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  m_velocityRecordingEnabled = configuration()->velocityRecordingEnabled();
  configuration()->velocityRecordingEnabledChanged().onNotify(
      this,
      [this] {
        m_velocityRecordingEnabled =
            configuration()->velocityRecordingEnabled();
      });

  const auto readOrnamentMode = [this]
  {
    m_ornamentsEnabled =
        configuration()->ornamentMode() == OrnamentMode::automatic;
  };
  readOrnamentMode();
  configuration()->ornamentModeChanged().onNotify(this, readOrnamentMode);

  staffModes()->modesChanged().onNotify(this,
                                        [this] { OnStaffModesChanged(); });

  dispatcher()->reg(this, "nav-first-control",
                    [this] { GoToTick(0, JumpReason::Rewind); });
  interactionProcessor()->itemClicked().onReceive(
      this, [this](const mu::engraving::EngravingItem *item)
      { GoToTick(item->tick().ticks(), JumpReason::Click); });
  RebuildFollower();
  GoToTick(0, JumpReason::Rewind);
}

namespace
{
void AppendNoteoffs(dgk::NoteEvents &output, const std::vector<int> &noteoffs,
                    TrackIndex track, const std::vector<int> &noteons)
{
  if (noteons.empty())
  {
    output.reserve(output.size() + noteoffs.size());
    std::transform(noteoffs.begin(), noteoffs.end(), std::back_inserter(output),
                   [&](int note) {
                     return NoteEvent{NoteEventType::noteOff, track, note, 0.f};
                   });
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
    std::optional<float> controllerVelocity, bool isLeftHand)
{
  if (transitions.empty())
    return;

  // The arpeggiated chords' notes, of every voice struck, are rolled
  // together: an arpeggio may span voices and staves.
  NoteEvents rolled;
  std::optional<RollDirection> roll;
  double rollTicksPerMs = 0.0;

  for (auto &[track, transition] : transitions)
  {
    const auto past = GetPastChord(transition);
    const auto present = GetPresentChord(transition);
    auto velocity = controllerVelocity;
    std::optional<OrnamentSchedule> strike;
    if (present)
    {
      if (!velocity.has_value())
      {
        if (present->GetVelocity() > 0.f)
          // A pre-recorded velocity takes precedence over everything else.
          velocity = present->GetVelocity();
        else
        {
          // Otherwise derive the base loudness from the score's dynamic
          // markings (p, mf, f, …) if any, falling back to a neutral default.
          // Accompaniment is then attenuated so the melody stands out: the
          // left hand, and — when the right hand has several voices — every
          // right-hand voice below the top (melody) one.
          constexpr float accompanimentGain = 0.7f;
          const float base = present->GetDynamicVelocity().value_or(0.5f);
          const bool isInnerRightHandVoice =
              m_rightHandUpperVoice.has_value() &&
              track.staffIndex() == m_rightHandUpperVoice->staffIndex() &&
              track.voiceIndex() != m_rightHandUpperVoice->voiceIndex();
          velocity = isLeftHand || isInnerRightHandVoice
                         ? base * accompanimentGain
                         : base;
        }
      }
      else if (m_velocityRecordingEnabled)
        present->SetVelocity(*velocity);

      strike = ScheduleChord(*present);
    }

    const bool ornamented =
        strike && (strike->steps.size() > 1 || !strike->closing.empty());
    std::optional<RollDirection> arpeggio;
    if (present && !ornamented)
      arpeggio = present->GetRoll();
    NoteEvents voiceOutput =
        ProceedOnTrack(track, past, std::move(strike), velocity.value_or(0.f));
    if (voiceOutput.empty())
      continue;
    if (arpeggio)
    {
      roll = arpeggio;
      rollTicksPerMs = TicksPerMs(*present);
      NoteEvents releases;
      for (NoteEvent &event : voiceOutput)
        (event.type == NoteEventType::noteOn ? rolled : releases)
            .push_back(std::move(event));
      if (!releases.empty())
        PostNoteEvents(std::move(releases), /*strum=*/false);
      continue;
    }
    // An ornament's first note goes out unspread: the next follows closely
    // and must find it sounding.
    PostNoteEvents(std::move(voiceOutput), /*strum=*/!ornamented);
  }

  if (!rolled.empty())
  {
    std::stable_sort(rolled.begin(), rolled.end(),
                     [up = *roll == RollDirection::up](const NoteEvent &a,
                                                       const NoteEvent &b)
                     { return up ? a.pitch < b.pitch : a.pitch > b.pitch; });
    PostNoteEvents(std::move(rolled), /*strum=*/false,
                   ArpeggioNoteMs(rollTicksPerMs));
  }

  PublishTransitions(transitions);
}

void OrchestrionSequencer::PublishTransitions(
    const std::map<TrackIndex, ChordTransition> &transitions)
{
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

muse::async::Channel<AutoPlayEvent> OrchestrionSequencer::HandNoteEvents() const
{
  return m_handNoteEvent;
}

muse::async::Channel<EventVariant> OrchestrionSequencer::OutputEvent() const
{
  return m_outputEvent;
}

const IChord *OrchestrionSequencer::GetNextChord(TrackIndex track) const
{
  for (const auto *voice : m_allVoices)
    if (voice->track == track)
      return voice->GetFutureChord();
  return nullptr;
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

muse::async::Channel<int, JumpReason>
OrchestrionSequencer::AboutToJumpPosition() const
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
  m_ornamentThreadMembers.cv.notify_one();
  m_pedalThread.join();
  m_noteThread.join();
  m_ornamentThread.join();
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

/**
 * When the machine lets go of the hand's chord: the earliest of its voices'
 * release ticks (see VoiceSequencer::GetNextReleaseTick).
 */
std::optional<dgk::Tick>
GetReleaseTick(const OrchestrionSequencer::HandVoices &hand)
{
  std::optional<dgk::Tick> result;
  for (const auto &voice : hand)
    if (const auto tick = voice->GetNextReleaseTick())
      result = result ? std::min(*result, *tick) : *tick;
  return result;
}
} // namespace

void OrchestrionSequencer::OnInputEvent(NoteEventType type, int pitch,
                                        std::optional<float> velocity)
{
  if (type == NoteEventType::noteOn ? IsNotesKey(pitch)
                                    : m_playedPitches.count(pitch) > 0)
    return OnPitchInput(type, pitch, std::move(velocity));
  std::optional<bool> isLeftHand;
  if (type == NoteEventType::noteOn)
    isLeftHand = PerformerHand(pitch);
  else if (m_rightHand.pressedKey == pitch)
    isLeftHand = false;
  else if (m_leftHand.pressedKey == pitch)
    isLeftHand = true;
  if (isLeftHand)
    OnGesture(type, *isLeftHand, pitch, std::move(velocity),
              /*byPerformer=*/true);
}

void OrchestrionSequencer::OnHandInputEvent(NoteEventType type, bool isLeftHand,
                                            std::optional<float> velocity)
{
  if (Mode(isLeftHand) == StaffMode::mute)
    return;
  // The keys the machine plays the hands with, below and above middle C.
  OnGesture(type, isLeftHand, isLeftHand ? 59 : 60, std::move(velocity),
            /*byPerformer=*/false);
}

StaffMode OrchestrionSequencer::Mode(bool isLeftHand) const
{
  return staffModes()->mode(isLeftHand ? 1 : 0);
}

bool OrchestrionSequencer::IsPerformed(bool isLeftHand) const
{
  const StaffMode mode = Mode(isLeftHand);
  return mode == StaffMode::notes || mode == StaffMode::rhythm;
}

bool OrchestrionSequencer::HandIn(bool isLeftHand, StaffMode mode) const
{
  return !(isLeftHand ? m_leftHand : m_rightHand).voices.empty() &&
         Mode(isLeftHand) == mode;
}

bool OrchestrionSequencer::IsNotesKey(int pitch) const
{
  const bool rightNotes = HandIn(false, StaffMode::notes);
  const bool leftNotes = HandIn(true, StaffMode::notes);
  if (!rightNotes && !leftNotes)
    return false;
  const bool rightRhythm = HandIn(false, StaffMode::rhythm);
  const bool leftRhythm = HandIn(true, StaffMode::rhythm);
  constexpr int minorThird = 3;
  if (leftRhythm)
    // Below the right hand's notes.
    return pitch > m_staffRanges[0].first - minorThird;
  if (rightRhythm)
    // Above the left hand's.
    return pitch < m_staffRanges[1].second + minorThird;
  return true;
}

std::optional<bool> OrchestrionSequencer::PerformerHand(int pitch) const
{
  const auto plays = [this](bool isLeftHand)
  { return HandIn(isLeftHand, StaffMode::rhythm); };
  const bool right = plays(false);
  const bool left = plays(true);
  if (right && left)
    return pitch < 60;
  if (right || left)
    return left;
  return std::nullopt;
}

void OrchestrionSequencer::OnGesture(NoteEventType type, bool isLeftHand,
                                     int key, std::optional<float> velocity,
                                     bool byPerformer)
{
  const auto loopEnabled =
      projectSession()->masterNotation()->playback()->loopBoundaries().enabled;
  OnInputEventRecursive(type, isLeftHand, key, std::move(velocity), loopEnabled,
                        byPerformer);

  if (m_footPedalling)
    // The foot decides when the pedal lifts.
    return;

  // Make sure to release the pedal once this part is over: nothing sounding
  // and nothing left to play in any voice that plays. Idle hands alone must
  // not lift it, or a staccato bass would never ring through its beat.
  auto partOver = true;
  for (const bool left : {false, true})
    if (Mode(left) != StaffMode::mute)
      for (const auto &voice : (left ? m_leftHand : m_rightHand).voices)
        if (voice->GetNextMatchingTick(NoteEventType::noteOff) ||
            voice->GetNextMatchingTick(NoteEventType::noteOn))
          partOver = false;
  if (m_pedalDown && type == NoteEventType::noteOff && partOver)
    PostPedalEvent(PedalEvent{m_instrument, false});
}

void OrchestrionSequencer::AllNotesOff()
{
  // Release through the regular input-event path, so that voice cursors,
  // chord-transition subscribers (score highlighting) and the take recording
  // all see an ordinary key release.
  for (const bool left : {false, true})
    if (const auto key = (left ? m_leftHand : m_rightHand).pressedKey)
      OnGesture(NoteEventType::noteOff, left, *key, std::nullopt,
                /*byPerformer=*/false);
  ReleasePlayedPitches();
  // Note-offs alone don't silence a piano while the sustain pedal is down.
  if (m_pedalDown)
    CutPedal();
  // Playing has stopped: there is no performance to follow any more, and
  // the Follow staves must not play on, gliding on the last tempo. The
  // follower expects the next note where the staves stopped.
  m_tempo.reset();
  m_tempoTick.reset();
  ResetFollower();
}

void OrchestrionSequencer::OnPitchInput(NoteEventType type, int pitch,
                                        std::optional<float> velocity)
{
  if (type == NoteEventType::noteOff)
  {
    const auto it = m_playedPitches.find(pitch);
    if (it == m_playedPitches.end())
      return;
    PostNoteEvents({NoteEvent{NoteEventType::noteOff, it->second, pitch, 0.f}},
                   /*strum=*/false);
    m_playedPitches.erase(it);
    if (m_wrongNotes.erase(pitch))
      PublishWrongNotes();
    ReleaseFollowedChords();
    return;
  }

  ++m_inputSerial;
  const double nowMs = NowMs();
  const TrackIndex track = NotesTrack();
  NoteEvents events;
  if (const auto it = m_playedPitches.find(pitch); it != m_playedPitches.end())
    // Struck again without a release in between: restrike.
    events.emplace_back(NoteEventType::noteOff, it->second, pitch, 0.f);
  // A controller without velocity (the computer keyboard) plays at the
  // neutral loudness SendTransitions defaults to.
  events.emplace_back(NoteEventType::noteOn, track, pitch,
                      velocity.value_or(0.5f));
  m_playedPitches.insert_or_assign(pitch, track);
  PostNoteEvents(std::move(events), /*strum=*/false);

  // The tempo is the performance's, which the Rhythm staves tell too.
  m_follower.OnNoteOn(pitch, nowMs,
                      m_tempo.ready() && m_tempo.speed() > 0.0
                          ? std::make_optional(m_tempo.speed())
                          : std::nullopt);
}

TrackIndex OrchestrionSequencer::NotesTrack() const
{
  // The staves are one instrument's: any of their tracks sounds the same.
  const auto &hand = HandIn(false, StaffMode::notes) ? m_rightHand : m_leftHand;
  return hand.voices.empty() ? TrackIndex{}
                             : hand.voices.front()->track.track();
}

void OrchestrionSequencer::ReleaseFollowedChords()
{
  // Not before the chord is nearly over: between the notes of a trill, or of
  // a note struck again, no key is held either.
  const auto now = PerformedTempoNow();
  if (!now)
    return;
  std::map<TrackIndex, ChordTransition> transitions;
  for (const bool left : {false, true})
    if (HandIn(left, StaffMode::notes))
      transitions.merge(PrepareStaffTransitions(
          (left ? m_leftHand : m_rightHand).voices,
          [&](VoiceSequencer &voice) -> std::optional<ChordTransition>
          {
            const IChord *chord = voice.GetSoundingChord();
            if (!chord)
              return std::nullopt;
            const auto begin = chord->GetBeginTick().withRepeats;
            const auto end = chord->GetEndTick().withRepeats;
            if (now->tick < begin + 0.75 * (end - begin))
              return std::nullopt;
            const std::vector<int> pitches = chord->GetPitches();
            if (std::any_of(pitches.begin(), pitches.end(), [this](int pitch)
                            { return m_playedPitches.count(pitch) > 0; }))
              return std::nullopt;
            // Past its end: on to the rest before the next.
            return voice.FollowTo(chord->GetEndTick().withRepeats);
          }));
  if (!transitions.empty())
    PublishTransitions(transitions);
}

void OrchestrionSequencer::FollowTo(int tickWithRepeats)
{
  std::map<TrackIndex, ChordTransition> transitions;
  for (const bool left : {false, true})
    if (HandIn(left, StaffMode::notes))
      transitions.merge(PrepareStaffTransitions(
          (left ? m_leftHand : m_rightHand).voices, [&](VoiceSequencer &voice)
          { return voice.FollowTo(tickWithRepeats); }));
  if (!transitions.empty())
    PublishTransitions(transitions);
}

void OrchestrionSequencer::OnFollowed(const FollowerThread::Followed &followed)
{
  if (m_followedTick != followed.tick)
  {
    m_followedTick = followed.tick;
    FollowTo(followed.tick);
    ObserveTick(followed.tick, followed.ms);
  }
  else
    // Struck again where the performer is: the voices released meanwhile
    // (see ReleaseFollowedChords) are back on it.
    FollowTo(followed.tick);
  if (followed.matched || !m_playedPitches.count(followed.pitch))
    // Right, or already released.
    return;
  // Against the chords where the performer is: those the Notes staves sound.
  std::vector<const IChord *> chords;
  for (const bool left : {false, true})
    if (HandIn(left, StaffMode::notes))
      for (const auto &voice : (left ? m_leftHand : m_rightHand).voices)
        if (const IChord *chord = voice->GetSoundingChord())
          chords.push_back(chord);
  m_wrongNotes[followed.pitch] = std::move(chords);
  PublishWrongNotes();
}

void OrchestrionSequencer::PublishWrongNotes()
{
  std::vector<const IChord *> chords;
  for (const auto &[pitch, against] : m_wrongNotes)
    for (const IChord *chord : against)
      if (std::find(chords.begin(), chords.end(), chord) == chords.end())
        chords.push_back(chord);
  m_wrongNotesChannel.send(chords);
}

muse::async::Channel<std::vector<const IChord *>>
OrchestrionSequencer::WrongNotes() const
{
  return m_wrongNotesChannel;
}

void OrchestrionSequencer::RebuildFollower()
{
  std::vector<const std::vector<ScoreEvent> *> staves;
  for (const bool left : {false, true})
    if (HandIn(left, StaffMode::notes))
      staves.push_back(&m_staffEvents[left ? 1 : 0]);
  m_follower.Follow(Merge(staves), m_nominalTicksPerMs);
  RebuildAutoPlayNotes();
  ResetFollower();
}

void OrchestrionSequencer::RebuildAutoPlayNotes()
{
  m_autoPlayNotes.clear();
  for (const bool left : {false, true})
    if (HandIn(left, StaffMode::notes))
      for (const ScoreNote &note : m_staffNotes[left ? 1 : 0])
      {
        m_autoPlayNotes.push_back(
            {note.onTick, NoteEventType::noteOn, note.pitch});
        m_autoPlayNotes.push_back(
            {note.offTick, NoteEventType::noteOff, note.pitch});
      }
  // Releases first: a note struck again where it ends is released first.
  std::stable_sort(m_autoPlayNotes.begin(), m_autoPlayNotes.end(),
                   [](const AutoPlayNote &a, const AutoPlayNote &b)
                   {
                     return a.tick != b.tick
                                ? a.tick < b.tick
                                : a.type == NoteEventType::noteOff &&
                                      b.type == NoteEventType::noteOn;
                   });
  m_nextAutoPlayNote = 0;
}

void OrchestrionSequencer::ResetFollower()
{
  m_followedTick.reset();
  // The performer is somewhere else now.
  if (!m_wrongNotes.empty())
  {
    m_wrongNotes.clear();
    PublishWrongNotes();
  }
  const std::vector<ScoreEvent> *const events = m_follower.Events();
  if (!events)
    return;
  // Where the next note is due: the earliest of the Notes staves' future
  // chords.
  std::optional<Tick> next;
  for (const bool left : {false, true})
    if (HandIn(left, StaffMode::notes))
      for (const auto &voice : (left ? m_leftHand : m_rightHand).voices)
        if (const auto chord = voice->GetFutureChord();
            chord && (!next || chord->GetBeginTick() < *next))
          next = chord->GetBeginTick();
  const auto tick = next ? next->withRepeats : 0;
  const auto it =
      std::lower_bound(events->begin(), events->end(), tick,
                       [](const ScoreEvent &e, int t) { return e.tick < t; });
  m_follower.Reset(static_cast<size_t>(it - events->begin()));
  // The automatic player plays on from there too.
  m_nextAutoPlayNote = static_cast<size_t>(
      std::lower_bound(m_autoPlayNotes.begin(), m_autoPlayNotes.end(), tick,
                       [](const AutoPlayNote &n, int t)
                       { return n.tick < t; }) -
      m_autoPlayNotes.begin());
}

void OrchestrionSequencer::ReleasePlayedPitches()
{
  NoteEvents events;
  for (const auto &[pitch, track] : m_playedPitches)
    events.emplace_back(NoteEventType::noteOff, track, pitch, 0.f);
  m_playedPitches.clear();
  if (!events.empty())
    PostNoteEvents(std::move(events), /*strum=*/false);
  if (!m_wrongNotes.empty())
  {
    m_wrongNotes.clear();
    PublishWrongNotes();
  }
}

void OrchestrionSequencer::OnSustainPedal(bool down)
{
  m_footDown = down;
  if (m_footPedalling && down != m_pedalDown)
    PostPedalEvent(PedalEvent{m_instrument, down}, true);
}

void OrchestrionSequencer::SetFootPedalling(bool on)
{
  if (on == m_footPedalling)
    return;
  m_footPedalling = on;
  if (on)
  {
    // The pedal goes where the foot has it.
    if (m_footDown != m_pedalDown)
      PostPedalEvent(PedalEvent{m_instrument, m_footDown}, true);
  }
  else if (m_pedalDown)
    // The score's pedalling picks up from the next gesture.
    PostPedalEvent(PedalEvent{m_instrument, false});
}

std::map<TrackIndex, ChordTransition>
OrchestrionSequencer::PrepareStaffTransitions(
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

void OrchestrionSequencer::OnInputEventRecursive(NoteEventType type,
                                                 bool isLeftHand, int key,
                                                 std::optional<float> velocity,
                                                 bool loop, bool byPerformer)
{
  ++m_inputSerial;
  auto &hand = isLeftHand ? m_leftHand : m_rightHand;
  if (type == NoteEventType::noteOn)
    hand.pressedKey = key;
  else if (hand.pressedKey == key)
    hand.pressedKey.reset();
  else
    return;

  const auto cursorTick =
      GetCursorTick(hand.voices, type).value_or(m_finalTick);

  const auto &loopBoundaries =
      projectSession()->masterNotation()->playback()->loopBoundaries();

  if (loop && type == NoteEventType::noteOn &&
      loopBoundaries.loopOutTick.ticks() > 0 &&
      cursorTick.withoutRepeats >= loopBoundaries.loopOutTick.ticks())
  {
    if (Mode(isLeftHand) == StaffMode::follow)
      // Not a Follow staff's to take the loop back: it waits at the loop's
      // end for the performer to.
      return;
    WrapLoop(loopBoundaries.loopInTick.ticks(), cursorTick,
             [&] { return GetCursorTick(hand.voices, type); });
    return OnInputEventRecursive(type, isLeftHand, key, std::move(velocity),
                                 false, byPerformer);
  }

  const std::map<TrackIndex, ChordTransition> transitions =
      PrepareStaffTransitions(hand.voices,
                              [&](VoiceSequencer &voice) { //
                                return voice.OnInputEvent(type, cursorTick);
                              });

  Finally maybeRewind{[this, doRewind = byPerformer && transitions.empty()]
                      {
                        if (doRewind)
                          GoToTick(0, JumpReason::ScoreEnd);
                      }};

  // Notify which hand just played — for the beginner-help key animation, and
  // (with the raw controller velocity) for the dynamics scoring. Reached only
  // after the press/release bookkeeping confirmed a genuine event, so it
  // fires exactly once per processed note-on/note-off.
  m_handNoteEvent.send(
      AutoPlayEvent{type, isLeftHand,
                    type == NoteEventType::noteOn ? velocity : std::nullopt});

  if (type == NoteEventType::noteOn)
    ObserveOnset(isLeftHand, transitions);

  SendTransitions(transitions, std::move(velocity), isLeftHand);

  UpdatePedal();
}

namespace
{
/**
 * The chord a hand is holding: the most recently struck of its sounding chords,
 * or null if it holds none.
 */
const IChord *GetHeldChord(const OrchestrionSequencer::HandVoices &voices)
{
  const IChord *held = nullptr;
  for (const auto &voice : voices)
    if (const auto chord = voice->GetSoundingChord();
        chord && (!held || held->GetBeginTick() < chord->GetBeginTick()))
      held = chord;
  return held;
}
} // namespace

void OrchestrionSequencer::UpdatePedal()
{
  if (m_footPedalling)
    return;
  // The spans, as [first, last) indices, within which every hand that is down
  // stands — a hand standing within a span when the chord it holds overlaps
  // it. A hand holding a note across a pedal change stands within both spans;
  // one still holding an upbeat stands within none of the first bar's.
  std::optional<std::pair<size_t, size_t>> common;
  for (const auto *hand : {&m_rightHand, &m_leftHand})
  {
    if (!hand->pressedKey)
      continue;
    const auto held = GetHeldChord(hand->voices);
    if (!held)
      continue;
    const auto begin = held->GetBeginTick().withRepeats;
    const auto end = held->GetEndTick().withRepeats;
    const auto first = std::find_if(
        m_pedalSequence.begin(), m_pedalSequence.end(),
        [&](const PedalSpan &span) { return span.offTick > begin; });
    const auto last =
        std::find_if(first, m_pedalSequence.end(),
                     [&](const PedalSpan &span) { return span.onTick >= end; });
    const std::pair<size_t, size_t> range{first - m_pedalSequence.begin(),
                                          last - m_pedalSequence.begin()};
    common = common ? std::make_pair(std::max(common->first, range.first),
                                     std::min(common->second, range.second))
                    : range;
  }

  if (!common)
    // No hand is down: the pedal stays as it is.
    return;
  if (common->first >= common->second)
  {
    // Hands in different spans, or in none.
    if (m_pedalSpan)
      PostPedalEvent(PedalEvent{m_instrument, false});
    return;
  }
  if (m_pedalSpan && common->first <= *m_pedalSpan &&
      *m_pedalSpan < common->second)
    // Still within the span the pedal is down in.
    return;
  // Down in the earliest common span, i.e. where the note was struck; a
  // re-pedal if the pedal was down in another.
  m_pedalSpan = common->first;
  PostPedalEvent(PedalEvent{m_instrument, true});
}

void OrchestrionSequencer::GoToTick(int tick, JumpReason reason)
{
  m_aboutToJumpPosition.send(tick, reason);
  // The position jumps: the tempo estimate starts over — but for a loop's
  // wrap, which the performance carries on through (see WrapLoop) — and
  // whatever ornament was under way is cut.
  if (reason != JumpReason::LoopWrap)
  {
    m_tempo.reset();
    m_tempoTick.reset();
  }
  CancelOrnaments();
  {
    // A muted staff stays where it is, out of sight: it has no position to
    // show until it plays again (see OnStaffModesChanged).
    std::map<TrackIndex, ChordTransition> transitions;
    for (const bool left : {false, true})
      if (Mode(left) != StaffMode::mute)
        transitions.merge(PrepareStaffTransitions(
            (left ? m_leftHand : m_rightHand).voices,
            [&](VoiceSequencer &voice) { return voice.GoToTick(tick); }));
    SendTransitions(std::move(transitions));
  }
  CutPedal();
  m_autoPlayTick = tick;
  // A click, a rewind, ...: the performer is to be followed from there.
  ResetFollower();
}

void OrchestrionSequencer::SkipHandTo(bool isLeftHand, int tickWithRepeats)
{
  if (Mode(isLeftHand) == StaffMode::mute)
    return;
  auto &hand = isLeftHand ? m_leftHand : m_rightHand;
  if (hand.pressedKey)
    OnGesture(NoteEventType::noteOff, isLeftHand, *hand.pressedKey,
              std::nullopt, /*byPerformer=*/false);
  SendTransitions(
      PrepareStaffTransitions(hand.voices, [&](VoiceSequencer &voice)
                              { return voice.SkipTo(tickWithRepeats); }),
      std::nullopt, isLeftHand);
}

void OrchestrionSequencer::WrapLoop(
    int loopInTick, const Tick &before,
    const std::function<std::optional<Tick>()> &next)
{
  GoToTick(loopInTick, JumpReason::LoopWrap);
  // The performance carries on through the wrap: its tempo estimate too,
  // moved back with the position.
  const auto after = next();
  if (!after)
  {
    m_tempo.reset();
    m_tempoTick.reset();
    return;
  }
  const int delta = after->withRepeats - before.withRepeats;
  m_tempo.shift(delta);
  if (m_tempoTick)
    *m_tempoTick += delta;
}

std::vector<const VoiceSequencer *> OrchestrionSequencer::PlayingVoices() const
{
  std::vector<const VoiceSequencer *> voices;
  for (const bool left : {false, true})
    if (Mode(left) != StaffMode::mute)
      for (const auto &voice : (left ? m_leftHand : m_rightHand).voices)
        voices.push_back(voice.get());
  return voices;
}

void OrchestrionSequencer::OnStaffModesChanged()
{
  if (projectSession()->masterNotation().get() != m_notation)
    // Another score's: this sequencer is on its way out.
    return;
  const std::array<StaffMode, 2> modes{Mode(false), Mode(true)};
  if (modes == m_modes)
    return;

  // Where the hands that played until now were: the earliest chord they
  // have to play next.
  std::optional<Tick> from;
  for (const bool left : {false, true})
    if (m_modes[left ? 1 : 0] != StaffMode::mute)
      for (const auto &voice : (left ? m_leftHand : m_rightHand).voices)
        if (const auto chord = voice->GetFutureChord();
            chord && (!from || chord->GetBeginTick() < *from))
          from = chord->GetBeginTick();

  AllNotesOff();
  m_modes = modes;
  RebuildFollower();
  GoToTick(from ? from->withoutRepeats : 0, JumpReason::Realign);
}

void OrchestrionSequencer::GoToPrevNoteonTick()
{
  std::optional<Tick> latest;
  for (const auto *voice : PlayingVoices())
  {
    const auto tick = voice->GetPreviousNoteonTick();
    if (tick && (!latest || *tick > *latest))
      latest = tick;
  }
  GoToTick(latest ? latest->withoutRepeats : 0, JumpReason::Step);
}

void OrchestrionSequencer::GoToNextNoteonTick()
{
  std::optional<Tick> earliest;
  for (const auto *voice : PlayingVoices())
  {
    const auto tick = voice->GetNextNoteonTick();
    if (tick && (!earliest || *tick < *earliest))
      earliest = tick;
  }
  if (earliest)
    GoToTick(earliest->withoutRepeats, JumpReason::Step);
}

std::optional<NextAutoPlayEvents> OrchestrionSequencer::WhatToPlayNext()
{
  struct Candidate
  {
    int tick; // with repeats: when the gesture is due
    NoteEventType type;
    bool isLeftHand;
  };

  std::vector<Candidate> candidates;
  for (bool isLeftHand : {false, true})
  {
    // What the performer would play, the player plays in their stead: not a
    // Follow staff, which follows the player as it would the performer — nor
    // a Notes staff, whose notes it plays as keys, below.
    if (!IsPerformed(isLeftHand) || Mode(isLeftHand) == StaffMode::notes)
      continue;
    auto &hand = isLeftHand ? m_leftHand : m_rightHand;
    for (auto eventType : {NoteEventType::noteOff, NoteEventType::noteOn})
    {
      const auto tick = eventType == NoteEventType::noteOn
                            ? GetCursorTick(hand.voices, eventType)
                            // A staccato chord is let go of early.
                            : GetReleaseTick(hand.voices);
      if (!tick)
        continue;
      const int due = eventType == NoteEventType::noteOn
                          ? AutoPlayStrikeTick(hand, *tick)
                          : tick->withRepeats;
      candidates.push_back({due, eventType, isLeftHand});
    }
  }

  const bool notesLeft = m_nextAutoPlayNote < m_autoPlayNotes.size();
  if (candidates.empty() && !notesLeft)
    return std::nullopt;

  int minTick = std::numeric_limits<int>::max();
  for (const auto &c : candidates)
    minTick = std::min(minTick, c.tick);
  if (notesLeft)
    minTick = std::min(minTick, m_autoPlayNotes[m_nextAutoPlayNote].tick);

  NextAutoPlayEvents result;
  result.deltaTicks = minTick - m_autoPlayTick;

  // Collect all events at the minimum tick, noteoffs first.
  for (auto type : {NoteEventType::noteOff, NoteEventType::noteOn})
    for (const auto &c : candidates)
      if (c.tick == minTick && c.type == type)
        (c.isLeftHand ? result.leftHandEvent : result.rightHandEvent) = c.type;
  for (; m_nextAutoPlayNote < m_autoPlayNotes.size() &&
         m_autoPlayNotes[m_nextAutoPlayNote].tick == minTick;
       ++m_nextAutoPlayNote)
    result.notes.emplace_back(m_autoPlayNotes[m_nextAutoPlayNote].type,
                              m_autoPlayNotes[m_nextAutoPlayNote].pitch);

  m_autoPlayTick = minTick;
  return result;
}

int OrchestrionSequencer::AnticipationTicks(const Hand &hand,
                                            const Tick &strike) const
{
  int ticks = 0;
  for (const auto &voice : hand.voices)
  {
    const IChord *chord = voice->GetFutureChord();
    if (!chord || chord->GetBeginTick().withRepeats != strike.withRepeats)
      continue;
    // Back from the schedule's time to ticks at the tempo it was timed with.
    const double ms = ScheduleChord(*chord).anticipation.count() / 1000.0;
    ticks =
        std::max(ticks, static_cast<int>(std::lround(ms * TicksPerMs(*chord))));
  }
  return ticks;
}

int OrchestrionSequencer::AutoPlayStrikeTick(const Hand &hand,
                                             const Tick &strike)
{
  using namespace std::chrono;
  // Grace notes fall before the beat: like a performer, strike the chord
  // they lead to early by their length, so that the chord itself lands on it.
  const int target = strike.withRepeats - AnticipationTicks(hand, strike);
  int due = target;
  for (const auto &voice : hand.voices)
  {
    const IChord *chord = voice->GetFutureChord();
    if (!chord || chord->GetBeginTick().withRepeats != strike.withRepeats)
      continue;
    std::unique_lock lock{m_ornamentThreadMembers.mutex};
    const auto it = m_tracks.find(voice->track);
    if (it == m_tracks.end() || !it->second.stepEnd ||
        it->second.schedule.cycleBegin >= it->second.schedule.cycleEnd)
      continue;
    // The hand is trilling. The step boundaries ahead, in the player's
    // ticks: the trill's notes were timed at the hand's tempo, so that is the
    // rate to count them back with. The events played last went at
    // m_autoPlayTick, which is now, give or take the timer's lateness.
    const TrackSchedule &entry = it->second;
    const OrnamentSchedule &schedule = entry.schedule;
    const double ticksPerMs = TicksPerMs(*chord);
    const auto ticksOf = [&](microseconds duration)
    { return duration.count() / 1000.0 * ticksPerMs; };
    const double untilStepEnd = std::max(
        0.0, duration<double, std::milli>(*entry.stepEnd - steady_clock::now())
                 .count());
    double boundary = m_autoPlayTick + untilStepEnd * ticksPerMs;
    size_t next = entry.next;
    // Walk the boundaries up to the target and settle on the nearest.
    double nearest = boundary;
    double stepTicks = 0.0;
    while (true)
    {
      if (next >= schedule.cycleEnd)
        next = schedule.cycleBegin;
      stepTicks = ticksOf(schedule.steps[next++].duration);
      if (boundary >= target || stepTicks <= 0.0)
        break;
      const double following = boundary + stepTicks;
      if (std::abs(following - target) <= std::abs(boundary - target))
        nearest = following;
      boundary = following;
      if (boundary >= target)
        break;
    }
    // Aim a little before the boundary, not at it: a timer a hair late would
    // find the next step begun and wait it out. Arriving early costs
    // nothing, the gesture taking effect at the boundary anyway.
    due = static_cast<int>(std::lround(nearest - stepTicks / 3.0));
    break;
  }
  return std::max(due, m_autoPlayTick);
}

const std::map<TrackIndex, ChordTransition> &
OrchestrionSequencer::GetCurrentTransitions() const
{
  return m_transitions.val;
}

void OrchestrionSequencer::CutPedal()
{
  PostPedalEvent(PedalEvent{m_instrument, false});
  if (m_footPedalling && m_footDown)
    PostPedalEvent(PedalEvent{m_instrument, true}, true);
}

void OrchestrionSequencer::PostPedalEvent(PedalEvent event, bool byFoot)
{
  using namespace std::chrono;
  using namespace std::chrono_literals;
  // How long the pedal stays up at least, for the dampers to act on the
  // strings.
  constexpr auto minUpTime = 100ms;

  if (!event.on)
    m_pedalSpan.reset();
  auto &m = m_pedalThreadMembers;
  const auto now = steady_clock::now();
  {
    std::unique_lock lock{m.mutex};
    const auto lift = [&]
    {
      // A lift supersedes a pending press; the releases held back for that
      // press go out now.
      std::erase_if(m.queue,
                    [](const auto &entry) {
                      return std::holds_alternative<PedalEvent>(entry.event);
                    });
      m.queue.emplace_back(QueueEntry<EventVariant>{
          std::nullopt, PedalEvent{m_instrument, false}});
      m_pedalLiftTime = now;
      m_liftSerial = m_inputSerial;
    };
    if (!event.on)
      lift();
    else
    {
      if (m_pedalDown)
        // A re-pedal.
        lift();
      const auto pressTime = m_pedalLiftTime + minUpTime;
      m.queue.emplace_back(QueueEntry<EventVariant>{
          !byFoot && pressTime > now ? std::make_optional(pressTime)
                                     : std::nullopt,
          std::move(event)});
    }
  }
  m.cv.notify_one();
  m_pedalDown = event.on;
}

void OrchestrionSequencer::PostNoteEvents(NoteEvents events, bool strum,
                                          std::optional<double> rollMs)
{
  using namespace std::chrono;

  // A release withdraws its note's strike still to come, of a roll or a
  // strum: sounding after the release, the note would hang.
  {
    auto &m = m_noteThreadMembers;
    std::unique_lock lock{m.mutex};
    const auto size = m.queue.size();
    m.queue.erase(
        std::remove_if(m.queue.begin(), m.queue.end(),
                       [&](const QueueEntry<NoteEvent> &entry)
                       {
                         return entry.event.type == NoteEventType::noteOn &&
                                std::any_of(
                                    events.begin(), events.end(),
                                    [&](const NoteEvent &e)
                                    {
                                      return e.type == NoteEventType::noteOff &&
                                             e.track == entry.event.track &&
                                             e.pitch == entry.event.pitch;
                                    });
                       }),
        m.queue.end());
    if (m.queue.size() != size)
    {
      lock.unlock();
      m.cv.notify_one();
    }
  }

  // The releases of notes struck since the pedal was last lifted are held back
  // while a press is pending: the pedal is to catch them, and it must stay up
  // its full time first. Held back, they go out right after the press — down,
  // the pedal sustains a released note rather than the release cutting it.
  {
    auto &m = m_pedalThreadMembers;
    std::unique_lock lock{m.mutex};
    const auto pressPending =
        std::any_of(m.queue.begin(), m.queue.end(),
                    [](const auto &entry)
                    {
                      const auto pedal = std::get_if<PedalEvent>(&entry.event);
                      return pedal && pedal->on;
                    });
    const auto heldBack = [&](const NoteEvent &event)
    {
      if (!pressPending || event.type != NoteEventType::noteOff)
        return false;
      const auto it = m_strikeSerial.find({event.track.value, event.pitch});
      return it != m_strikeSerial.end() && it->second >= m_liftSerial;
    };
    NoteEvents releases;
    std::copy_if(events.begin(), events.end(), std::back_inserter(releases),
                 heldBack);
    if (!releases.empty())
    {
      events.erase(std::remove_if(events.begin(), events.end(), heldBack),
                   events.end());
      m.queue.emplace_back(
          QueueEntry<EventVariant>{std::nullopt, std::move(releases)});
      lock.unlock();
      m.cv.notify_one();
    }
  }
  for (const auto &event : events)
    if (event.type == NoteEventType::noteOn)
      m_strikeSerial[{event.track.value, event.pitch}] = m_inputSerial;
    else
      m_strikeSerial.erase({event.track.value, event.pitch});
  if (events.empty())
    return;

  if (rollMs && events.size() > 1)
  {
    // In the order given, the first now.
    std::vector<QueueEntry<NoteEvent>> entries;
    const auto now = steady_clock::now();
    auto delayUs = 0.0;
    for (const NoteEvent &event : events)
    {
      entries.push_back(QueueEntry<NoteEvent>{
          now + microseconds{static_cast<long long>(delayUs)}, event});
      if (event.type == NoteEventType::noteOn)
        delayUs += *rollMs * 1000.0;
    }
    auto &m = m_noteThreadMembers;
    {
      std::unique_lock lock{m.mutex};
      for (auto &entry : entries)
        m.queue.push_back(std::move(entry));
    }
    m.cv.notify_one();
    return;
  }

  const auto numNoteons =
      std::count_if(events.begin(), events.end(), [](const auto &event)
                    { return event.type == NoteEventType::noteOn; });
  if (numNoteons < 2 || !strum)
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
      m.queue.push_back(std::move(entry));
  }

  m.cv.notify_one();
}

void OrchestrionSequencer::ObserveOnset(
    bool isLeftHand, const std::map<TrackIndex, ChordTransition> &transitions)
{
  if (Mode(isLeftHand) == StaffMode::follow)
    return;
  // The staff's voices are played by the same gesture: one onset, the
  // furthest struck.
  std::optional<int> tick;
  for (const auto &[track, transition] : transitions)
    if (const IChord *present = GetPresentChord(transition))
    {
      const int begin = present->GetBeginTick().withRepeats;
      tick = tick ? std::max(*tick, begin) : begin;
    }
  if (tick)
    ObserveTick(*tick, NowMs());
}

void OrchestrionSequencer::ObserveTick(int tick, double nowMs)
{
  // After a pause — the next note long overdue — the wound-down estimate says
  // nothing about the tempo playing resumes at: start over.
  m_tempo.heartbeat(nowMs);
  if (m_tempo.isCoasting())
  {
    m_tempo.reset();
    m_tempoTick.reset();
  }
  // Not before the onset fed last: the other hand's half of a chord struck
  // by both, or a hand lagging the other, tells the tempo nothing new.
  if (m_tempoTick && tick <= *m_tempoTick)
    return;
  m_tempoTick = tick;
  m_tempo.addObservation(nowMs, tick);
}

double OrchestrionSequencer::TicksPerMs(const IChord &chord) const
{
  const double nominalBpm =
      chord.GetNominalBpm() > 0.0 ? chord.GetNominalBpm() : 120.0;
  const double nominal = nominalBpm * ticksPerQuarter / 60000.0;
  if (!m_tempo.ready() || m_tempo.speed() <= 0.0)
    return nominal;
  // A young estimate can be wild: kept within reason of the score's tempo.
  return std::clamp(m_tempo.speed(), nominal / 4.0, nominal * 4.0);
}

std::optional<PerformedTempo> OrchestrionSequencer::PerformedTempoNow()
{
  const double nowMs = NowMs();
  // Between onsets the estimate glides on, and coasts to a stop once the
  // next one is overdue.
  m_tempo.heartbeat(nowMs);
  if (!m_tempo.ready())
    return std::nullopt;
  return PerformedTempo{m_tempo.positionAt(nowMs), m_tempo.speed(),
                        m_tempo.isCoasting()};
}

double OrchestrionSequencer::NowMs() const
{
  using namespace std::chrono;
  return duration<double, std::milli>(steady_clock::now() - m_epoch).count();
}

void OrchestrionSequencer::QueueOrnamentStep(
    std::deque<QueueEntry<OrnamentStepDue>> &queue, OrnamentStepDue due)
{
  // In due-time order, so that the thread waits on the earliest.
  const auto pos = std::find_if(queue.begin(), queue.end(),
                                [&](const QueueEntry<OrnamentStepDue> &entry) {
                                  return entry.time && *entry.time > due.time;
                                });
  queue.insert(pos, QueueEntry<OrnamentStepDue>{due.time, due});
}

OrnamentSchedule OrchestrionSequencer::ScheduleChord(const IChord &chord) const
{
  const Ornament *ornament = chord.GetOrnament();
  if (!ornament || !m_ornamentsEnabled)
    return PlainChord(chord.GetPitches());
  const int nominalTicks =
      chord.GetEndTick().withRepeats - chord.GetBeginTick().withRepeats;
  return ScheduleOrnament(*ornament, chord.GetPitches(), nominalTicks,
                          TicksPerMs(chord));
}

std::vector<int>
OrchestrionSequencer::Begin(TrackIndex track, TrackSchedule &entry,
                            OrnamentSchedule schedule, float velocity,
                            std::chrono::steady_clock::time_point start)
{
  assert(!schedule.steps.empty());
  entry.schedule = std::move(schedule);
  entry.next = 1;
  entry.velocity = velocity;
  const TimedOrnamentStep &first = entry.schedule.steps.front();
  entry.sounding = first.pitches;
  entry.stepEnd.reset();
  if (first.duration.count() > 0)
  {
    entry.stepEnd = start + first.duration;
    QueueOrnamentStep(m_ornamentThreadMembers.queue,
                      OrnamentStepDue{track, entry.generation, *entry.stepEnd});
  }
  return first.pitches;
}

NoteEvents
OrchestrionSequencer::ProceedOnTrack(TrackIndex track, const IChord *past,
                                     std::optional<OrnamentSchedule> strike,
                                     float velocity)
{
  const auto now = std::chrono::steady_clock::now();
  const bool ornamented =
      strike && (strike->steps.size() > 1 || !strike->closing.empty());
  NoteEvents events;
  auto &m = m_ornamentThreadMembers;
  bool queued = false;
  {
    std::unique_lock lock{m.mutex};
    std::vector<int> noteoffs;
    std::vector<int> noteons;
    float noteonVelocity = velocity;
    const auto it = m_tracks.find(track);
    if (it != m_tracks.end() && it->second.stepEnd)
    {
      // Mid-way through a timed step: the move waits for it to end.
      it->second.move = TrackSchedule::Move{std::move(strike), velocity, false};
      return {};
    }
    else if (it != m_tracks.end() && !it->second.schedule.closing.empty())
    {
      // The chord's closing notes sound first; the move follows them.
      TrackSchedule &entry = it->second;
      OrnamentSchedule closing;
      closing.steps = std::move(entry.schedule.closing);
      noteoffs = std::move(entry.sounding);
      noteonVelocity = entry.velocity;
      noteons = Begin(track, entry, std::move(closing), entry.velocity, now);
      entry.move = TrackSchedule::Move{std::move(strike), velocity, true};
      queued = true;
    }
    else
    {
      if (it != m_tracks.end())
      {
        noteoffs = std::move(it->second.sounding);
        m_tracks.erase(it);
      }
      else if (past)
        noteoffs = past->GetPitches();

      if (strike && ornamented)
      {
        TrackSchedule &entry = m_tracks[track];
        entry.generation = ++m_ornamentGeneration;
        noteons = Begin(track, entry, std::move(*strike), velocity, now);
        queued = entry.stepEnd.has_value();
      }
      else if (strike)
        // A plain chord, struck plainly: nothing to keep track of.
        noteons = strike->steps.front().pitches;
    }
    // A lane split off a track sounds on the track (see TrackIndex).
    AppendNoteoffs(events, noteoffs, track.track(), noteons);
    if (!noteons.empty())
      AppendNoteons(events, noteons, track.track(), noteonVelocity);
  }
  if (queued)
    m.cv.notify_one();
  return events;
}

void OrchestrionSequencer::OnOrnamentStepDue(OrnamentStepDue due)
{
  auto &m = m_ornamentThreadMembers;
  std::unique_lock lock{m.mutex};
  const auto it = m_tracks.find(due.track);
  if (it == m_tracks.end() || it->second.generation != due.generation)
    return; // cancelled, or struck anew, in the meantime
  TrackSchedule &entry = it->second;
  entry.stepEnd.reset();
  const OrnamentSchedule &schedule = entry.schedule;
  const bool cycling = schedule.cycleBegin < schedule.cycleEnd;
  const bool exhausted = entry.next >= schedule.steps.size() && !cycling;

  NoteEvents events;
  const auto silence = [&](const std::vector<int> &pitches)
  {
    for (const int pitch : pitches)
      events.emplace_back(NoteEventType::noteOff, due.track.track(), pitch,
                          0.f);
  };
  const auto sound = [&](const std::vector<int> &pitches, float velocity)
  {
    for (const int pitch : pitches)
      events.emplace_back(NoteEventType::noteOn, due.track.track(), pitch,
                          velocity);
  };

  if (entry.move && (!entry.move->afterClosing || exhausted))
  {
    // The gesture that came during the step takes effect now.
    TrackSchedule::Move move = std::move(*entry.move);
    entry.move.reset();
    silence(entry.sounding);
    if (move.strike)
      sound(Begin(due.track, entry, std::move(*move.strike), move.velocity,
                  due.time),
            move.velocity);
    else
      m_tracks.erase(it);
  }
  else
  {
    if (entry.next >= schedule.steps.size())
    {
      if (!cycling)
        return; // a held step ends with a gesture, not a timer: not reached
      entry.next = schedule.cycleBegin;
    }
    const TimedOrnamentStep &step = schedule.steps[entry.next++];
    silence(entry.sounding);
    sound(step.pitches, entry.velocity);
    entry.sounding = step.pitches;
    if (step.duration.count() > 0)
    {
      entry.stepEnd = due.time + step.duration;
      QueueOrnamentStep(
          m.queue, OrnamentStepDue{due.track, due.generation, *entry.stepEnd});
    }
  }

  // Sent under the lock: a gesture arriving now must find these sounding, not
  // silence them before they sound.
  m_outputEvent.send(std::move(events));
}

void OrchestrionSequencer::CancelOrnaments()
{
  NoteEvents noteoffs;
  auto &m = m_ornamentThreadMembers;
  {
    std::unique_lock lock{m.mutex};
    for (const auto &[track, entry] : m_tracks)
      for (const int pitch : entry.sounding)
        noteoffs.emplace_back(NoteEventType::noteOff, track.track(), pitch,
                              0.f);
    m_tracks.clear();
    m.queue.clear();
  }
  // The thread may be waiting on an entry just removed.
  m.cv.notify_one();
  if (!noteoffs.empty())
    PostNoteEvents(std::move(noteoffs));
}

} // namespace dgk
