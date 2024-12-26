#pragma once

#include "IOrchestrionSequencer.h"
#include "OrchestrionNotation/IOrchestrionNotationInteraction.h"
#include "OrchestrionTypes.h"
#include "internal/VoiceSequencer.h"
#include <actions/actionable.h>
#include <actions/iactionsdispatcher.h>
#include <array>
#include <async/asyncable.h>
#include <chrono>
#include <context/iglobalcontext.h>
#include <functional>
#include <memory>
#include <midi/imidiinport.h>
#include <modularity/ioc.h>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <unordered_set>
#include <vector>

namespace dgk
{
class OrchestrionSequencer : public IOrchestrionSequencer,
                             public muse::Injectable,
                             public muse::async::Asyncable,
                             public muse::actions::Actionable
{
  muse::Inject<mu::context::IGlobalContext> globalContext;
  muse::Inject<muse::actions::IActionsDispatcher> dispatcher;
  muse::Inject<muse::midi::IMidiInPort> midiInPort;
  muse::Inject<orchestrion::IOrchestrionNotationInteraction>
      orchestrionNotationInteraction;

public:
  using Hand = std::vector<std::unique_ptr<VoiceSequencer>>;

  OrchestrionSequencer(int track, Hand rightHand, Hand leftHand, PedalSequence);
  ~OrchestrionSequencer();

  void OnInputEvent(const NoteEvent &inputEvent) override;
  //! Returns noteoffs that were pending.
  void GoToTick(int tick) override;
  int GetTrack() const override;

  muse::async::Channel<int, ChordActivationChange>
  ChordActivationChanged() const override;
  muse::async::Channel<EventVariant> OutputEvent() const override;

private:
  using OptTimePoint =
      std::optional<std::chrono::time_point<std::chrono::steady_clock>>;

  template <typename EventType> struct QueueEntry
  {
    OptTimePoint time;
    EventType event;
  };

  template <typename EventType> struct ThreadMembers
  {
    std::queue<QueueEntry<EventType>> queue;
    std::mutex mutex;
    std::condition_variable cv;
  };

  template <typename EventType>
  static std::thread MakeThread(OrchestrionSequencer &self,
                                ThreadMembers<EventType> &members,
                                std::function<void(EventType)> cb);

  void OnMidiEventReceived(const muse::midi::Event &event);
  void OnInputEventRecursive(const NoteEvent &inputEvent, bool loop);
  void PostPedalEvent(PedalEvent event);
  void PostNoteEvents(NoteEvents events);

  const int m_track;

  Hand m_rightHand;
  Hand m_leftHand;
  const std::vector<const VoiceSequencer *> m_allVoices;
  const PedalSequence m_pedalSequence;
  PedalSequence::const_iterator m_pedalSequenceIt;

  std::thread m_pedalThread;
  ThreadMembers<PedalEvent> m_pedalThreadMembers;
  std::thread m_noteThread;
  ThreadMembers<NoteEvent> m_noteThreadMembers;

  bool m_finished = false;
  bool m_pedalDown = false;
  std::mt19937 m_rng{0};
  std::uniform_int_distribution<int> m_delayDist{0, 50000};   // microseconds
  std::uniform_int_distribution<int> m_velocityDist{70, 130}; // percents

  std::unordered_set<int> m_pressedKeys;

  muse::async::Channel<int, ChordActivationChange> m_chordActivationChanged;
  muse::async::Channel<EventVariant> m_outputEvent;
};
} // namespace dgk
