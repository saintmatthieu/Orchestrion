# Re-threading: handover (Windows, macOS)

Branch `re-threading`, as of `b058deb` (2026-11-16). The design is in
[threading.md](threading.md); this note is what you need to carry on from
another machine. Delete it before merging.

## Why, in two lines

The GUI thread waits on the render thread at every frame of an animation
(30–90 % of the time while playing, measured on Linux), and ~99 % when the
window is covered (XWayland throttles hidden windows). Everything between a
gesture and its sound used to go through it, so playing felt late and
uneven.

## What's done (all tested on Linux only)

| Commit | What |
| --- | --- |
| `c5c3608` | `PlayThread`: tasks posted or timed on `steady_clock`; `Wait()`. Offline rendering: the main thread itself. |
| `94e85b9` | Sequencer, `FollowPlayer`, auto-play scheduling on the play thread, behind `SequencerProxy` (the GUI's `IOrchestrionSequencer`). |
| `260ec96` | MIDI: `InputThread` (GestureControllers) listens to the device; gestures go to `GestureInputConnector` on the play thread via `IGestureInput::setReceiver`. |
| `d78fd55` | X11: `RawKeyboard` reads XInput2 raw key events on its own thread. CI: `libxi-dev`, `libxi6`. |
| `bbd561d` | Crash on quitting: services read null in destructors during IoC reset. |
| `b058deb` | `OutputEvent` to the audio engine only; GUI observers use `MainThreadOutputEvent` (relayed); jump cut is `AllNotesOffEvent` in line with the notes. |

How it was checked on Linux: both tutorials live; offline renders
byte-identical run to run (and to the pre-change build, but for 6.8 ms
earlier Follow notes); MIDI via the virtual keyboard; real keys via XTest;
score shelf open → digits go to the search box, don't play; Ctrl+Q exits
cleanly; loop wraps keep the loop's first chord.

## Rules the code relies on (break one and it deadlocks or races)

1. **The play thread never waits for the main thread.** The main thread may
   wait for it (`PlayThread::Wait`, used to create/destroy what lives there).
2. **No thread the main thread may wait for sends kors messages to the main
   thread.** A kors send can block while the main thread processes its queue
   or while its queue is full (up to 200 ms), and the first send of a
   channel between two threads takes a lock the main thread holds while
   processing. Use queued Qt calls (`QMetaObject::invokeMethod(...,
   Qt::QueuedConnection)`) instead; see `SequencerProxy::Relay`,
   `GestureInput::Dispatch`, `ComputerKeyboard::onKey`.
3. **Subscriptions are made and let go of on the thread they deliver on.**
   Play-thread subscribers use a separate `Asyncable` member created and
   reset inside `Wait()` (see `SequencerProxy`, `AutomaticOrchestrionPlayer`,
   `GestureInput::m_midiDeviceSubscriptions`).
4. **Tasks posted for an object are guarded** (`m_alive` weak pointer, or
   `FollowerThread::Life`): pending timed tasks outlive `Wait()`.
5. **In destructors, don't use `dgk::Inject` services**: quitting nulls them
   first. Keep a `shared_ptr` from construction (see
   `ComputerKeyboardGestureController::m_keyboard`).
6. **Offline rendering stays on the main thread** (`offlineRendering.audioPath`
   non-empty): the play thread is the main thread, no input thread, no raw
   keyboard. Renders must stay identical run to run.

## First thing on each platform: does it build and play?

Nothing was compiled on Windows or macOS. Likely spots:

- `ComputerKeyboard.cpp`: `RawKeyboard.h` is included only under
  `Q_OS_LINUX`, and `m_raw` is a `std::shared_ptr<RawKeyboard>` (forward
  declared) so the other platforms needn't define it. `raw()` returns null
  there, and Qt's key path works as before.
- `GestureControllers/CMakeLists.txt`: `RawKeyboard_lin.cpp` and `X11 Xi` under
  `OS_IS_LIN` only.
- `OrchestrionSequencer.cpp`: `SetThreadPriority(m_noteThread...)` under
  `OS_IS_WIN` was there before; untouched.
- `InputThread` uses `muse::async::processMessages(thread id)`; MIDI on
  Windows (`WinMidiInPort`, callback thread) and macOS (`CoreMidiInPort`,
  CoreMIDI's thread) sends from its own thread like ALSA's, so the input
  thread should receive it the same way. **Check with a real MIDI keyboard.**

Then play: computer keyboard (Qt path there), MIDI keyboard, Follow mode with
the window covered, the tutorials (Help ▸ Tutorials…), quit.

## Step 5: the computer keyboard off the GUI thread

`RawKeyboard.h` is platform-neutral already: `Create(callback)`, which
returns null where unavailable; `SetPlayable(bool)`; the callback is
`(char key, bool pressed)`, with the key as `QKeyEvent::key()` has it
(letters upper-case, digits `'0'`–`'9'`). Add a `RawKeyboard_win.cpp` and a
`RawKeyboard_mac.mm`, list them in CMake per platform, and drop the
`Q_OS_LINUX` guards in `ComputerKeyboard.cpp`. The gating
(`ComputerKeyboardImpl::updatePlayable`: application active and
`!keysAreForSomethingElse()`, on focus/state changes and every 100 ms), the
spontaneous-event filter and the receivers are shared. Only on the
`offscreen` platform must it not be created.

The callback's contract, as the X11 one keeps it:
- **Presses** count only while `m_playable`; **releases** always count.
- **Repeats** are dropped (the controller dedupes too).
- **Shift, Ctrl, Alt (Cmd on macOS?)** held: nothing counts, as in Qt's
  filter.
- **The character** comes from the current layout, as Qt would report it.
  With AZERTY, the digit row gives `&é"'...` both ways; keep that consistent.

### Windows: Raw Input — done

`RawKeyboard_win.cpp`, as below; played by hand on Windows. The X11 one is
now `RawKeyboard_lin.cpp`, its stop fd in its `Connection`.

- A thread with a message-only window (`HWND_MESSAGE`) and
  `RegisterRawInputDevices` for the keyboard (usage page 1, usage 6) with
  `RIDEV_INPUTSINK`, so it gets input whatever has the focus (and gate with
  `m_playable`).
- `WM_INPUT` → `GetRawInputData` → `RAWKEYBOARD`: `VKey`, `Flags & RI_KEY_BREAK`
  for release.
- Character: `MapVirtualKeyExW(vk, MAPVK_VK_TO_CHAR, GetKeyboardLayout(<the
  GUI thread's id>))` (the layout is per thread: ask for the main thread's),
  then upper-case.
- Modifiers: `GetAsyncKeyState(VK_SHIFT/VK_CONTROL/VK_MENU) & 0x8000`.
- Repeats: raw input sends repeated makes without a flag. Keep the set of
  keys down and drop a make for a key already down.
- Stop: `PostMessage(hwnd, WM_QUIT/WM_CLOSE)` from the destructor, then join.
- No permission needed.

### macOS: needs a decision first

There is no permission-free way to read keys off the main thread:
- A `CGEventTap` (listen-only) on its own run loop thread, or an
  `IOHIDManager` on its own thread, both need the **Input Monitoring**
  permission since 10.15: a system prompt, and the user granting it in
  System Settings.
- `NSEvent` monitors run on the main thread, so they gain nothing.

Options:
- (a) Keep Qt's path on macOS (no step 5 there).
- (b) Make it opt-in: a setting that asks for the permission
  (`IOHIDRequestAccess(kIOHIDRequestTypeListenEvent)`) and falls back to
  Qt's path while it's denied.

Measure first. If the GUI thread waits as much on macOS (Metal, no XWayland
throttling), (b) may be worth it; it may well not.

## Step 6: priorities (planned, after step 5)

See threading.md, step 6. For the other platforms:
- **Windows:** `SetThreadPriority(..., THREAD_PRIORITY_ABOVE_NORMAL)` for the
  play, note, ornament, pedal and input threads (and the raw keyboard's).
  Maybe MMCSS (`AvSetMmThreadCharacteristicsW(L"Games")`) for the play
  thread.
- **macOS:** `pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)`,
  called from each thread itself (first thing in its function).
- **Linux:** `SCHED_FIFO` ~10, falling back to rtkit, then nothing.
- **Not** the follower thread.

## Known gaps

- **Velocity recording:** `MuseChord::SetVelocity` runs on the play thread,
  and saving reads it on the main thread (a race if saving mid-performance).
- **MIDI out** (`OrchestrionEventProcessor`) and the take recorder's
  timestamps are still the main thread's (relayed), as late as it is busy.
- **`SequencerProxy::WhatToPlayNext`** waits for the play thread. Nothing on
  the main thread calls it; keep it that way.
- **Untested on Linux:** a real MIDI keyboard; holding a key while the
  window loses focus (the release must still come); a native Wayland session
  (`main.cpp` forces xcb, so XWayland).

## Testing without the Linux tooling

`tools/run_testflow.sh` and `--test-case-render` are Linux-only, but the
testflow runner itself isn't: `Orchestrion --test-case <script>` runs a
script in the app and exits 0/1/2. Useful throwaway scripts (from this work):
put them anywhere, `require("steps/Orchestrion.js")` resolves against the
installed `testflowscripts`.

`MidiPath.js`, which plays the virtual MIDI keyboard (a gesture every
300 ms):

```js
/*
 * @testflow test
 */
var Orchestrion = require("steps/Orchestrion.js")
var testCase = { name: "MIDI path", description: "Plays the virtual MIDI keyboard", steps: [
    {name: "Play", func: function() {
        Orchestrion.command("command://orchestrion/midi-device", {id: "virtual"})
        api.testflow.sleep(1000)
        for (var i = 0; i < 16; ++i) {
            var pitch = i % 2 ? 72 : 48
            Orchestrion.command("command://orchestrion/midi-note", {pitch: pitch, velocity: 0.8})
            api.testflow.sleep(200)
            Orchestrion.command("command://orchestrion/midi-note", {pitch: pitch, velocity: 0})
            api.testflow.sleep(100)
        }
        api.testflow.sleep(1500)
    }},
]}
function main() { api.testflow.runTestCase(testCase) }
```

`LoopWrap.js`: the loop on, auto-play through two wraps. The loop's first
chord must sound at each wrap.

```js
/*
 * @testflow test
 */
var Orchestrion = require("steps/Orchestrion.js")
var testCase = { name: "Loop wrap", description: "Plays with the loop on", steps: [
    {name: "Play", func: function() {
        Orchestrion.toggleLoop()
        Orchestrion.rewind()
        api.testflow.sleep(500)
        Orchestrion.togglePlay()
        api.testflow.sleep(45000)
        Orchestrion.togglePlay()
        api.testflow.sleep(1000)
    }},
]}
function main() { api.testflow.runTestCase(testCase) }
```

For real keys, `KeysWait.js` (a step that only does
`api.testflow.sleep(16000)`), with keys injected from outside:
`SendInput` on Windows, `CGEventPost` or AppleScript `key code` on macOS.
These are the events the raw keyboard reads; Qt's filter leaves them to it.

To measure the GUI thread's waits: on Linux, `/proc/<pid>/task/*/wchan`
sampled every 2 ms. On Windows, use Windows Performance Recorder/Analyzer
(CPU usage, precise: wait analysis of the GUI thread). On macOS, use
Instruments (System Trace).
