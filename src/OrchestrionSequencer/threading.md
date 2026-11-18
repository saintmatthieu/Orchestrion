# Playing off the GUI thread

## Why

Everything between a gesture and its sound currently goes through the GUI
thread: Qt's key events, the MIDI input (MuseScore's MIDI thread hands its
events to the main thread), the Follow staves' 16 ms poll (`FollowPlayer`) and
the automatic player's onsets (`AutomaticOrchestrionPlayer`, `QTimer`s). The
GUI thread is not always free to run them. With the threaded scene graph, it
waits on the render thread at every frame of an animation.

Measured on 2026-11-15 (Linux, XWayland, `/proc` wchan sampling of a release
AppImage), the GUI thread waited on the render thread:

- **30–90 %** of the time while the score animated during playback, with the
  window focused;
- **99 %** of the time with the window covered by another one, because
  XWayland throttles a hidden window's frames. The Follow staff then played
  slow and uneven.

In a test app with an animated QML scene, a key press reached Qt's GUI thread
with an extra delay of 7 ms median, 12 ms p90 and 18 ms max. A thread reading
XInput2 raw key events on a connection of its own got the same presses with a
delay of 0.5 ms median and 1 ms max.

## Design

### The play thread

`PlayThread` (`internal/PlayThread.h`) is a thread with a queue of tasks, each
run as soon as possible or at a given `steady_clock` time. It doesn't process kors
queues: the objects living on it are told of the main thread's changes by
posted tasks, and their own channels reach the receivers on the play thread
synchronously.

- The sequencer, the `FollowPlayer` and the automatic player's scheduling live
  on it. Their timers become play-thread timers instead of `QTimer`s.
- **It never waits for the main thread**, it only posts to it. The main thread
  may therefore wait for it, e.g. to let go of a sequencer (see Lifetime).
- **It never sends kors messages to the main thread**, only queued Qt calls:
  the first message of a channel from one thread to another registers a
  queue under a lock that the main thread holds while it processes its kors
  queues. If that main thread is waiting for the play thread meanwhile, they
  deadlock.
- **Offline rendering:** time is virtual on the main thread only. Under
  `--test-case-render`, the play thread is the main thread itself, with tasks
  posted as queued Qt calls and timers as `QTimer`s, so renders stay identical
  from one run to the next.

### The sequencer's front, on the main thread

GUI code keeps an `IOrchestrionSequencer`, implemented by a proxy
(`SequencerProxy`), and by it only: the interface is what the main thread and
the audio engine see of the sequencer. Playing it (`OnInputEvent`,
`OnSustainPedal`, `AllNotesOff`, ...) is `OrchestrionSequencer`'s own, for what
lives on the play thread with it (the players, the gesture connector).

- **Commands** (`GoToTick`, `GoToEnd`, ...) are posted to the play thread.
- **Notifications** (transitions, about-to-jump, hand note events, wrong
  notes, gestures) are relayed to the main thread as one ordered stream.
  Separate kors channels crossing threads don't keep their order relative to
  one another, and subscribers rely on `AboutToJumpPosition` coming before
  the jump's transitions. Each comes with when it happened (`EventTime`):
  what goes by time, judging the timing of onsets, goes by that rather than
  by when the busy main thread got to it.
- **Queries** are answered from what the stream brought:
  - `GetCurrentTransitions` returns a copy kept on the main thread.
  - `GetNextChord` gets each track's next chord computed on the play thread
    and carried with each batch of transitions.
  - `PerformedTempoNow` reads a snapshot of the tempo estimate, published
    under a mutex after each onset and extrapolated by the reader.
  - `GetAllVoices` and `RoleOfKey` only read what never changes, plus the
    staff modes, which the main thread owns.
- **What is to sound** goes to the synthesizers only, sent by one thread, the
  sequencer's output thread (a `PlayThread`), through `ISynthesizerFeed`:
  a lock-free ring per synthesizer, which it reads at the start of each
  block, on the audio callback's thread. Not a kors channel, whose
  subscriber, the synthesizer, lives on the audio engine's worker, which
  looks at its queue every 16 ms: up to 16 ms more for every note. The
  play, pedal and ornament threads queue their output for it
  (`QueueOutput`), in order of due time.
  With one sender, what is sent stays in order: a jump's cut
  (`AllNotesOffEvent`) drops what is queued and comes after all that was
  sent before it, before all that is sent after. Rendering offline, the
  output thread is the main thread, on its virtual clock. The main thread's
  observers (the pedal indicator, MIDI out) use `MainThreadOutputEvent`,
  which the proxy relays as queued Qt calls
  (`OrchestrionSequencer::SetOutputObserver`): no sequencer thread sends kors
  messages to the main thread.

### What the sequencer reads from the main thread today

The sequencer, while on the play thread, gets copies of these, updated by
posted tasks when they change:

- the staff modes;
- the loop boundaries;
- the ornament mode and the velocity-recording switch;
- the playback tempo and its multiplier (for the automatic player).

The action registrations (`nav-first-control`, play/stop) stay on the main
thread and post to the play thread.

Recording a chord's velocity marks the score's notes: the sequencer posts
`MuseChord::SetVelocity` to the main thread (`SetMainThreadPoster`), and reads
velocities from atomics, the saved one read at construction.

### Input

- **All gestures** reach the sequencer through `GestureInputConnector`, on the
  play thread. `IGestureInput` hands each one to it (`setReceiver`) on the
  thread the gesture comes from, and the connector posts it to the play
  thread. A take's replay sends through `IGestureInput::send`. What observes
  gestures on the main thread (the take recorder) listens to the sequencer's
  `Gestures`, relayed in order with what they cause.
- **MIDI:** `GestureInput` listens to the MIDI device on an `InputThread` of
  its own, which processes its kors queue every millisecond (kors has no
  wake-up hook). `VirtualMidiInPort::eventReceived` is the device's own
  channel, so the events reach it from the MIDI thread directly, as they reach
  the GUI's subscribers (the virtual keyboard's lights). Rendering offline,
  all of it stays on the main thread.
- **Computer keyboard, Linux/X11:** `RawKeyboard` reads XInput2 raw key
  events on the root window, on a thread with an X connection of its own.
  Raw events are delivered in addition to the normal ones, so Qt keeps its
  keys. (Selecting `XI_KeyPress` on Qt's window instead steals its keys.)
  Raw events don't say which window they are for, so the main thread tells
  the thread, through an atomic, whether presses play: the application is
  active and `keysAreForSomethingElse()` is false. It recomputes that when the
  focus or the application's state changes, and every 100 ms, since what the
  keys are for can change without the focus moving (the score shelf being
  shown). Releases always count. Key codes become characters with the XKB
  keymap (`XkbLookupKeySym`), as `QKeyEvent::key()` has them. A key goes from
  that thread to the gesture input through direct calls
  (`IComputerKeyboard::setReceiver`), not kors. Qt's key filter leaves the
  spontaneous key events to the raw keyboard and still plays synthesized ones
  (the tutorials' `DemoKeys`). Not on the offscreen platform (offline
  rendering).
- **Computer keyboard, Windows:** the same `RawKeyboard`, reading Raw Input
  (`RIDEV_INPUTSINK`) from a message-only window on a thread of its own, the
  characters in the main thread's keyboard layout.
- **Computer keyboard, macOS:** Qt's key events, on the main thread. Reading
  the keys off it (a `CGEventTap`, an `IOHIDManager`) needs the Input
  Monitoring permission: see What's left.

### Lifetime

The sequencer is created on the main thread, which reads the score, and handed
to the play thread. To let go of it, the main thread posts its detachment and
waits for the play thread to have run it. It is then destroyed on the main
thread, as `10a24a1` requires.

## Rules the code relies on

Break one and it deadlocks, races, or renders differ from one run to the next.

1. **The play thread never waits for the main thread.** The main thread may
   wait for it (`PlayThread::Wait`, used to create and destroy what lives
   there).
2. **No thread the main thread may wait for sends kors messages to the main
   thread.** A kors send can block while the main thread processes its queue
   or while its queue is full (up to 200 ms), and the first send of a
   channel between two threads takes a lock the main thread holds while
   processing. Use queued Qt calls instead (`QMetaObject::invokeMethod(...,
   Qt::QueuedConnection)`): see `SequencerProxy::Relay`,
   `ComputerKeyboard::onKey`.
3. **Subscriptions are made and let go of on the thread they deliver on.**
   Play-thread subscribers use a separate `Asyncable` member created and
   reset inside `Wait()` (see `SequencerProxy`, `AutomaticOrchestrionPlayer`,
   `GestureInput::m_midiDeviceSubscriptions`).
4. **Tasks posted for an object are guarded** (an `m_alive` weak pointer, or
   `FollowerThread::Life`): pending timed tasks outlive `Wait()`.
5. **Off the main thread and in destructors, don't use `dgk::Inject`
   services**: quitting sets them to null, on the main thread, before
   destroying them. Keep a `shared_ptr` from construction (see
   `ComputerKeyboardGestureController::m_keyboard`,
   `MidiDeviceGestureController::m_port`).
6. **Offline rendering stays on the main thread** (`offlineRendering.audioPath`
   non-empty): the play and output threads are the main thread, the follower
   works inline, there is no input thread and no raw keyboard. A real thread
   whose result the main thread uses makes renders flaky: the virtual clock
   jumps ahead while the main thread waits.
7. **One sender to the audio engine**: everything for it is queued for the
   output thread (`QueueOutput`), never sent from another thread.

## What's left

- **Thread priorities.** The play, output, ornament and pedal threads, the
  input thread and the raw keyboard's, above normal; not the follower thread,
  whose work grows with the score's length. Each calls
  `raiseTimedThreadPriority()` (`OrchestrionCommon/ThreadPriority.h`) as it
  starts; offline, the play and output threads are the main thread and
  aren't raised.
  - Linux: `SCHED_FIFO` at a low priority (~10, below PipeWire's 88), asked
    for directly (an `audio` group member's `RLIMIT_RTPRIO`) or through
    rtkit, staying at normal priority otherwise. `nice` can't be lowered
    without privilege, and MuseScore's `setThreadPriority` is a no-op under
    `SCHED_OTHER`.
  - Windows: `THREAD_PRIORITY_ABOVE_NORMAL`, maybe MMCSS for the play thread.
  - macOS (done): user-interactive QoS. Default QoS let the automatic
    player's timed waits wake up 4–5 ms late (up to 11.5), coalesced; now
    1 ms. Real-time scheduling (`THREAD_TIME_CONSTRAINT_POLICY`) gave
    0.03 ms, but the kernel demotes a thread that exceeds its computation
    budget, and the play thread's work isn't bounded: not taken.
- **The macOS keyboard** off the main thread needs the Input Monitoring
  permission (a system prompt, granted in System Settings). Measure first
  how much the main thread waits there (Metal, no XWayland throttling); if it
  is worth it, make it opt-in (`IOHIDRequestAccess`), falling back to Qt's
  keys while denied.
- **The pedal and ornament threads** are real threads in offline renders
  too: their timing there goes by the wall clock. Queued on the output
  thread's `PlayThread` instead, they would go by the virtual one.
- **MIDI out** (`OrchestrionEventProcessor`) is on the main thread (relayed),
  as late as it is busy.
- **Not run yet:** velocity recording (posted to the main thread, atomics in
  `MuseChord`); a real MIDI keyboard on Linux.
