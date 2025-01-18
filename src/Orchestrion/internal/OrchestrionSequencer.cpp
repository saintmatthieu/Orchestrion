#include "OrchestrionSequencer.h"
#include "IChord.h"
#include <algorithm>
#include <engraving/dom/note.h>
#include <iterator>
#include <notation/inotationmidiinput.h>
#include <numeric>

namespace dgk
{
namespace
{
auto MakeAllVoices(const OrchestrionSequencer::Hand &rightHand,
                   const OrchestrionSequencer::Hand &leftHand)
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

NoteEventType GetNoteEventType(const muse::midi::Event &event)
{
  return event.opcode() == muse::midi::Event::Opcode::NoteOn
             ? dgk::NoteEventType::noteOn
             : dgk::NoteEventType::noteOff;
}

int GetPitch(const muse::midi::Event &event) { return event.note(); }

float CompressVelocity(float velocity)
{
  // It's a little hard to play piano at a constant level. For now we hard-code
  // a compression. Making it parametric would be helpful, especially to use
  // harder compression for kids.
  constexpr auto minVelocity = 0.2f;
  return minVelocity + (1 - minVelocity) * velocity;
}

float GetVelocity(const muse::midi::Event &event)
{
  return event.velocity() / 128.0f;
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
                                           Hand rightHand, Hand leftHand,
                                           PedalSequence pedalSequence)
    : m_instrument{std::move(instrument)}, m_rightHand{std::move(rightHand)},
      m_leftHand{std::move(leftHand)},
      m_allVoices{MakeAllVoices(m_rightHand, m_leftHand)},
      m_pedalSequence{std::move(pedalSequence)},
      m_pedalSequenceIt{m_pedalSequence.begin()},
      m_pedalThread{MakeThread<PedalEvent>(*this, m_pedalThreadMembers,
                                           [this](PedalEvent event)
                                           { m_outputEvent.send(event); })},
      m_noteThread{MakeThread<NoteEvent>(
          *this, m_noteThreadMembers,
          [this](NoteEvent event) { m_outputEvent.send(NoteEvents{event}); })}
{
  dispatcher()->reg(this, "nav-first-control", [this] { GoToTick(0); });
  midiInPort()->eventReceived().onReceive(
      this, [this](const muse::midi::tick_t, const muse::midi::Event &event)
      { OnMidiEventReceived(event); });
  orchestrionNotationInteraction()->noteClicked().onReceive(
      this, [this](const mu::engraving::Note *note)
      { GoToTick(note->tick().ticks()); });
}

namespace
{
void AppendNoteoffs(dgk::NoteEvents &output, const std::vector<int> &noteoffs,
                    TrackIndex track, float velocity,
                    const std::vector<int> &noteons)
{
  if (noteons.empty())
  {
    output.reserve(output.size() + noteoffs.size());
    std::transform(
        noteoffs.begin(), noteoffs.end(), std::back_inserter(output),
        [&](int note)
        { return NoteEvent{NoteEventType::noteOff, track, note, velocity}; });
  }
  else
    // Only append noteoffs that aren't in `noteons`.
    for (const auto noteoff : noteoffs)
      if (std::find(noteons.begin(), noteons.end(), noteoff) == noteons.end())
        output.push_back(
            NoteEvent{NoteEventType::noteOff, track, noteoff, velocity});
}

void AppendNoteons(dgk::NoteEvents &output, const std::vector<int> &noteons,
                   TrackIndex track, float velocity)
{
  output.reserve(output.size() + noteons.size());
  std::transform(
      noteons.begin(), noteons.end(), std::back_inserter(output), [&](int note)
      { return NoteEvent{NoteEventType::noteOn, track, note, velocity}; });
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

void OrchestrionSequencer::SendChordTransition(TrackIndex track,
                                               ChordTransition transition,
                                               float velocity)
{
  std::vector<NoteEvent> voiceOutput;
  const auto noteons =
      transition.activatedChordRest && transition.activatedChordRest->AsChord()
          ? transition.activatedChordRest->AsChord()->GetPitches()
          : std::vector<int>{};
  if (transition.deactivatedChord)
    AppendNoteoffs(voiceOutput, transition.deactivatedChord->GetPitches(),
                   track, velocity, noteons);
  if (transition.activatedChordRest)
    AppendNoteons(voiceOutput, noteons, track, velocity);

  if (!voiceOutput.empty())
    PostNoteEvents(voiceOutput);

  m_chordTransitionTriggered.send(track, std::move(transition));
}

muse::async::Channel<TrackIndex, ChordTransition>
OrchestrionSequencer::ChordTransitionTriggered() const
{
  return m_chordTransitionTriggered;
}

muse::async::Channel<EventVariant> OrchestrionSequencer::OutputEvent() const
{
  return m_outputEvent;
}

void OrchestrionSequencer::OnMidiEventReceived(const muse::midi::Event &event)
{
  if (event.isChannelVoice20())
  {
    auto events = event.toMIDI10();
    for (auto &midi10event : events)
      OnMidiEventReceived(midi10event);
    return;
  }

  if (event.opcode() != muse::midi::Event::Opcode::NoteOn &&
      event.opcode() != muse::midi::Event::Opcode::NoteOff)
    return;

  OnInputEvent(GetNoteEventType(event), GetPitch(event),
               CompressVelocity(GetVelocity(event)));
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
auto GetNextNoteonTick(const OrchestrionSequencer::Hand &hand,
                       NoteEventType type)
{
  return std::accumulate(
      hand.begin(), hand.end(), std::optional<dgk::Tick>{},
      [&](const auto &acc, const auto &voice)
      {
        const auto tick = voice->GetNextTick(type);
        return tick.has_value()
                   ? std::make_optional(std::min(acc.value_or(*tick), *tick))
                   : acc;
      });
}
} // namespace

void OrchestrionSequencer::OnInputEvent(NoteEventType type, int pitch,
                                        float velocity)
{
  if (type == NoteEventType::noteOn)
    m_pressedKey = pitch;
  else if (m_pressedKey == pitch)
    m_pressedKey.reset();
  else
    return;

  const auto loopEnabled = globalContext()
                               ->currentMasterNotation()
                               ->playback()
                               ->loopBoundaries()
                               .enabled;
  OnInputEventRecursive(type, pitch, velocity, loopEnabled);

  // Make sure to release the pedal if we've reached the end of this part.
  if (m_pedalDown && !GetNextNoteonTick(m_rightHand, NoteEventType::noteOff) &&
      !GetNextNoteonTick(m_leftHand, NoteEventType::noteOff) &&
      type == NoteEventType::noteOff)
    PostPedalEvent(PedalEvent{m_instrument, false});
}

void OrchestrionSequencer::OnInputEventRecursive(NoteEventType type, int pitch,
                                                 float velocity, bool loop)
{

  auto &hand = pitch < 60 && !m_leftHand.empty() ? m_leftHand : m_rightHand;
  const auto cursorTick =
      GetNextNoteonTick(hand, type).value_or(GetFinalTick(m_allVoices));

  const auto &loopBoundaries =
      globalContext()->currentMasterNotation()->playback()->loopBoundaries();

  if (loop && type == NoteEventType::noteOn && loopBoundaries.loopOutTick > 0 &&
      cursorTick.withoutRepeats >= loopBoundaries.loopOutTick)
  {
    GoToTick(loopBoundaries.loopInTick);
    return OnInputEventRecursive(type, pitch, velocity, false);
  }

  for (auto &voiceSequencer : hand)
    SendChordTransition(voiceSequencer->track,
                        voiceSequencer->OnInputEvent(type, cursorTick),
                        velocity);

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

  const auto prevPedalSequenceIt = m_pedalSequenceIt;
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
  for (auto hand : {&m_rightHand, &m_leftHand})
    for (auto &sequencer : *hand)
      SendChordTransition(sequencer->track, sequencer->GoToTick(tick));

  m_outputEvent.send(PedalEvent{m_instrument, false});
  m_pedalSequenceIt = std::lower_bound(
      m_pedalSequence.begin(), m_pedalSequence.end(), tick,
      [](const auto &item, int tick) { return item.tick < tick; });
}

std::map<TrackIndex, const IChord *> OrchestrionSequencer::GetNextChords() const
{
  std::map<TrackIndex, const IChord *> nextChords;
  for (const auto voice : m_allVoices)
    nextChords[voice->track] = voice->GetNextChord();
  return nextChords;
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
