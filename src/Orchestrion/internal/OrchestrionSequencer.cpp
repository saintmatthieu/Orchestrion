#include "OrchestrionSequencer.h"
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

dgk::NoteEvent ToDgkNoteEvent(const muse::midi::Event &museEvent)
{
  const auto type = museEvent.opcode() == muse::midi::Event::Opcode::NoteOn
                        ? dgk::NoteEvent::Type::noteOn
                        : dgk::NoteEvent::Type::noteOff;
  const int channel = museEvent.channel();
  const int pitch = museEvent.note();
  const float velocity = museEvent.velocity() / 128.0f;
  return dgk::NoteEvent{type, channel, pitch, velocity};
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

OrchestrionSequencer::OrchestrionSequencer(int track, Hand rightHand,
                                           Hand leftHand,
                                           PedalSequence pedalSequence)
    : m_track{track}, m_rightHand{std::move(rightHand)},
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
  std::for_each(
      m_allVoices.begin(), m_allVoices.end(),
      [this](const VoiceSequencer *voiceSequencer)
      {
        voiceSequencer->ChordActivationChanged().onReceive(
            this, [this, voiceSequencer](ChordActivationChange change)
            { m_chordActivationChanged.send(voiceSequencer->track, change); });
      });
  orchestrionNotationInteraction()->elementClicked().onReceive(
      this,
      [this](const mu::notation::EngravingItem *item)
      {
        if (const auto note = dynamic_cast<const mu::engraving::Note *>(item))
          GoToTick(note->tick().ticks());
      });
}

muse::async::Channel<int /* track */, ChordActivationChange>
OrchestrionSequencer::ChordActivationChanged() const
{
  return m_chordActivationChanged;
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

  OnInputEvent(ToDgkNoteEvent(event));
}

OrchestrionSequencer::~OrchestrionSequencer()
{
  if (m_pedalDown)
  {
    PostPedalEvent(PedalEvent{m_track, false});
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
                       NoteEvent::Type type)
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

void OrchestrionSequencer::OnInputEvent(const NoteEvent &input)
{

  if (input.type == NoteEvent::Type::noteOn)
  {
    m_pressedKeys.insert(input.pitch);
  }
  else if (m_pressedKeys.count(input.pitch))
    m_pressedKeys.erase(input.pitch);

  const auto loopEnabled = globalContext()
                               ->currentMasterNotation()
                               ->playback()
                               ->loopBoundaries()
                               .enabled;
  OnInputEventRecursive(input, loopEnabled);

  // Make sure to release the pedal if we've reached the end of this part.
  if (m_pedalDown &&
      !GetNextNoteonTick(m_rightHand, NoteEvent::Type::noteOff) &&
      !GetNextNoteonTick(m_leftHand, NoteEvent::Type::noteOff) &&
      input.type == NoteEvent::Type::noteOff && m_pressedKeys.empty())
    PostPedalEvent(PedalEvent{m_track, false});
}

namespace
{
void Append(dgk::NoteEvents &output, const std::vector<int> &notes, int voice,
            float velocity, NoteEvent::Type type)
{
  std::transform(notes.begin(), notes.end(), std::back_inserter(output),
                 [&](int note)
                 { return NoteEvent{type, voice, note, velocity}; });
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

void OrchestrionSequencer::OnInputEventRecursive(const NoteEvent &input,
                                                 bool loop)
{

  auto &hand =
      input.pitch < 60 && !m_leftHand.empty() ? m_leftHand : m_rightHand;
  const auto cursorTick =
      GetNextNoteonTick(hand, input.type).value_or(GetFinalTick(m_allVoices));

  const auto &loopBoundaries =
      globalContext()->currentMasterNotation()->playback()->loopBoundaries();

  if (loop && input.type == NoteEvent::Type::noteOn &&
      loopBoundaries.loopOutTick > 0 &&
      cursorTick.withoutRepeats >= loopBoundaries.loopOutTick)
  {
    GoToTick(loopBoundaries.loopInTick);
    return OnInputEventRecursive(input, false);
  }

  std::vector<NoteEvent> output;
  for (auto &voiceSequencer : hand)
  {
    const auto next =
        voiceSequencer->OnInputEvent(input.type, input.pitch, cursorTick);
    output.reserve(output.size() + next.noteOffs.size() + next.noteOns.size());
    Append(output, next.noteOffs, voiceSequencer->voice, input.velocity,
           NoteEvent::Type::noteOff);
    Append(output, next.noteOns, voiceSequencer->voice, input.velocity,
           NoteEvent::Type::noteOn);
  };

  if (!output.empty())
    PostNoteEvents(output);

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
    if (input.type == NoteEvent::Type::noteOff)
    {
      // Just a release.
      PostPedalEvent(PedalEvent{m_track, false});
    }
    else
    {
      const auto &item = *(newPedalSequenceIt - 1);
      PostPedalEvent(PedalEvent{m_track, item.down});
      m_pedalSequenceIt = newPedalSequenceIt;
    }
  }
}

void OrchestrionSequencer::GoToTick(int tick)
{
  std::vector<NoteEvent> output;
  for (auto hand : {&m_rightHand, &m_leftHand})
  {
    for (auto &sequencer : *hand)
    {
      const auto next = sequencer->GoToTick(tick);
      output.reserve(output.size() + next.size());
      std::transform(next.begin(), next.end(), std::back_inserter(output),
                     [&](int note)
                     {
                       return NoteEvent{NoteEvent::Type::noteOff,
                                        sequencer->voice, note, 0.f};
                     });
    }
  }
  if (!output.empty())
    m_outputEvent.send(output);

  m_outputEvent.send(PedalEvent{m_track, false});
  m_pedalSequenceIt = std::lower_bound(
      m_pedalSequence.begin(), m_pedalSequence.end(), tick,
      [](const auto &item, int tick) { return item.tick < tick; });
}

int OrchestrionSequencer::GetTrack() const { return m_track; }

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
      m.queue.emplace(
          QueueEntry<PedalEvent>{std::nullopt, PedalEvent{m_track, false}});
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
                    { return event.type == NoteEvent::Type::noteOn; });
  if (numNoteons < 2)
  {
    m_outputEvent.send(std::move(events));
    return;
  }

  // Randomize the order of the notes and add random delays.
  std::shuffle(events.begin(), events.end(), m_rng);

  // Do not add unnecessary delay.
  std::vector<int> delays(events.size());
  std::generate(delays.begin(), delays.end(),
                [&] { return m_delayDist(m_rng); });
  const auto min = *std::min_element(delays.begin(), delays.end());
  std::transform(delays.begin(), delays.end(), delays.begin(),
                 [&](int delay) { return delay - min; });

  std::vector<QueueEntry<NoteEvent>> entries;
  entries.reserve(events.size());
  const auto now = steady_clock::now();
  for (auto i = 0u; i < events.size(); ++i)
  {
    auto &event = events[i];
    event.velocity =
        std::clamp(event.velocity * m_velocityDist(m_rng) / 100, 0.f, 1.f);
    entries.emplace_back(
        QueueEntry<NoteEvent>{now + microseconds{delays[i]}, std::move(event)});
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
