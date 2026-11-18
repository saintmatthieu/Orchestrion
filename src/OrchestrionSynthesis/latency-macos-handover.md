# Handover: lower latency on macOS

For an agent working on a Mac, on branch `latency-lowering`. Delete this file
once its tasks are done (or fold what's still true into
`src/OrchestrionSynthesis/gestureToSound.md`).

## Where things stand

Read `src/OrchestrionSynthesis/gestureToSound.md` first: a note's way from a
gesture to its sound, thread by thread. On this branch:

- **The synthesizers are fed on the audio thread** (`f24313c`): the sequencer's
  output thread pushes each event to a lock-free ring per synthesizer
  (`OrchestrionSequencer/ISynthesizerFeed.h`, `EventRing.h`), drained at the
  start of each block in `OrchestrionSynthesizerWrapper::process`. Before, a
  kors channel delivered them on the audio engine's worker, polled every
  16 ms.
- **A latency tracer** (temporary, `src/OrchestrionCommon/LatencyTrace.h`):
  `Orchestrion --latency-trace <dir>` stamps each note at each hand-off
  between threads and records every audio callback (start, end, frames);
  written to `<dir>` on quitting, with `meta.txt` (OS, driver, device, the
  sample rate and buffer size asked for and opened, those available). Read it
  with `tools/latency_trace.py <dir>` (`--notes` for one row per note). It
  doesn't measure what comes after the block leaves the app (the driver's
  and the device's buffering).

## What a trace on macOS showed

macOS 15.7, arm64, on battery below 20 %, driver `OSX`, 44.1 kHz, 1024 frames:

- Gesture note-on to its block: 10.8 ms p50, 20.9 p90, 23.2 max (one buffer:
  the fix works).
- Callbacks perfectly regular (23.22 ms), load 4 % p50, 20 % max: room for
  smaller buffers.
- **The automatic player fires its notes late**: 3.6 ms p50, 5.0 p90, 5.1 max
  after they were due (Linux: 0.17, 0.27, 1.1). Gestures, which wake the play
  thread, aren't (0.03 ms).
- Every computer-key release was dispatched twice (on Linux too): the
  application's event filter saw each key event once for the window and once
  for the item it passed it on to. Fixed on this branch; check on the Mac
  that a trace's `gesture note-off` count equals `gesture note-on`'s.

## Task 1: the direct CoreAudio driver, at the device's own rate

`OSX` is MuseScore's `OSXAudioDriver`
(`MuseScore/muse/framework/audio/driver/platform/osx/osxaudiodriver.mm`),
what the `audio/io/audioDriver` setting's macOS default, `CoreAudio`, makes
(`main/internal/audiodriverfactory.cpp`). It plays through an `AudioQueue` with
**two buffers of the block size enqueued** (line ~142), and sets the device's
buffer to the block size too: each block waits behind another whole buffer,
23 ms at 1024 frames, which the trace can't see.

Upstream added `OSXDirectAudioDriver` (`osxdirectaudiodriver.mm`, "removed
AudioQueue and added direct IO callback", May 2026): an IOProc on the device,
selected by the setting `CoreAudioDirect`. To do:

1. Make `CoreAudioDirect` Orchestrion's default on macOS, without forking
   MuseScore if possible: Orchestrion's configurations already set defaults
   with `muse::settings()->setDefaultValue` (e.g.
   `ExternalDevices/internal/ExternalDevicesConfiguration.cpp`); the key is
   `Settings::Key("audio", "io/audioDriver")`. Mind the order: it must come
   after `AudioConfiguration::init` sets its own default, and before the
   driver is created. A user's stored value wins over a default; Orchestrion
   has no UI for this setting, so there should be none, but check.
2. **The sample rate.** `prepareDeviceWithOutputSpec` (direct driver) sets the
   device's nominal rate to the closest to what is asked (the
   `audio/io/sampleRate` setting, 44100 by default): that changes the rate
   of the device for the whole system, and MacBooks' output usually runs at
   48 kHz. Orchestrion should ask for the device's current rate instead
   (unless the user chose one): read `kAudioDevicePropertyNominalSampleRate`
   and use that as the default, or make the driver keep the current rate.
   Linux has the same question (PipeWire at 48 kHz, Orchestrion at 44.1), to
   be settled the same way, so keep the "which rate" decision out of
   macOS-only code if you can (e.g. "the device's current rate, when the
   driver can tell" behind the driver).
3. Check that the `AudioWorkGroup` the direct driver joins and the engine's
   multithreaded track processing (`Mixer::processTrackChannels`) still
   behave; and that changing the output device in Orchestrion still works.

Verify with traces before and after (same session shape: a minute of keys,
a minute of automatic play, quit normally): `meta.txt` must say
`driver=` the direct driver and the rate opened; callbacks should stay
regular. The trace can't show the AudioQueue's buffer going away: a
loopback measurement can (a key's click into a microphone next to the
speaker, or the app's own `audio/io/measureInputLag` if it helps), else
reason from the code.

## Task 2: timers on time (thread QoS)

The automatic player's notes are timed on the play thread (`PlayThread`,
`condition_variable` waits until due); ornaments, graces and the pedal on the
ornament and pedal threads; everything to the synthesizers goes through the
output thread. A steady 5 ms ceiling on lateness, on battery, looks like
macOS timer coalescing of default-QoS threads. `threading.md`'s "What's left"
already plans thread priorities: on macOS
`pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)`, called by each
thread itself, for the play, output, ornament and pedal threads, the input
thread (`GestureControllers/internal/InputThread`) and the raw keyboard's
(none on macOS yet); not the follower thread.

Make it a small helper the threads call as they start (e.g. in
`OrchestrionCommon`), with the macOS branch done here; the Linux
(`SCHED_FIFO` ~10, or nothing without the right) and Windows
(`THREAD_PRIORITY_ABOVE_NORMAL`) branches are done on those machines.
`PlayThread` has no hook for it yet. Rendering offline (`--test-case-render`,
Linux only) the play and output threads are the main thread: don't touch
its priority.

First confirm the cause: the same automatic-play trace plugged in, and on
battery with Low Power Mode on and off. Then verify: the `played` step of
`auto note-on` should drop to Linux's level (well under 1 ms).

## Practicalities

- Build and run as in `CLAUDE.md` (macOS: install prefix is the build dir).
  Commit on `latency-lowering`, formatted with the repo's `.clang-format`
  (`clang-format --dry-run --Werror` on every touched `.h/.cpp`).
- Patches to MuseScore or the muse framework go on the fork branches (see
  `CLAUDE.md`, "Fork patches"); avoid them if Orchestrion can do it.
- Put the traces' numbers (before/after) in the commit messages.
