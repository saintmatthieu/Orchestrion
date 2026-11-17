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

GUI code keeps an `IOrchestrionSequencer` with the same interface, now
implemented by a proxy:

- **Commands** (`GoToTick`, `GoToEnd`, `AllNotesOff`, `OnInputEvent`, ...)
  are posted to the play thread.
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
- **`OutputEvent`** goes to the audio engine only, sent by one thread, the
  sequencer's output thread (a `PlayThread`): the play, pedal and ornament
  threads queue their output for it (`QueueOutput`), in order of due time.
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
- **Other platforms** keep Qt's key events for now. Candidates are a
  `CGEventTap` on macOS and Raw Input on Windows.

### Lifetime

The sequencer is created on the main thread, which reads the score, and handed
to the play thread. To let go of it, the main thread posts its detachment and
waits for the play thread to have run it. It is then destroyed on the main
thread, as `10a24a1` requires.

## Steps

1. `PlayThread`, with its render-mode variant, and its tests.
2. The sequencer, `FollowPlayer` and the automatic player on the play thread,
   behind the proxy. Input still comes from the GUI thread, posted. Check that
   renders are identical to before, and that a covered window no longer slows
   the Follow staves.
3. MIDI input straight to the play thread.
4. The Linux raw keyboard thread.
5. macOS and Windows keyboards.
6. Priorities: the play thread and the sequencer's output, ornament and pedal
   threads (and the input thread) above normal. On Linux that takes
   `SCHED_FIFO`, at a low priority (~10, below PipeWire's 88), asked for
   directly (an `audio` group member's `RLIMIT_RTPRIO`) or through rtkit,
   staying at normal priority otherwise: `nice` can't be lowered without
   privilege, and MuseScore's `setThreadPriority` is a no-op under
   `SCHED_OTHER`. `THREAD_PRIORITY_ABOVE_NORMAL` on Windows, the
   user-interactive QoS class on macOS. Not the follower thread, whose work
   grows with the score's length.
