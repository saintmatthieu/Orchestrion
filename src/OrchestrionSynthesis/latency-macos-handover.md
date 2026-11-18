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

## Task 1: the direct CoreAudio driver, at the device's own rate (done)

Done on 2026-11-18, see `gestureToSound.md` (after the table of delays):
`CoreAudioDirect` is the default (`OrchestrionSynthesisModule::onPreInit`,
see "Learned on macOS" below: no factory decorator needed, and MuseScore's
own fallback to `CoreAudio` kept), `DeviceFollowingAudioDriver` opens the
device at its own rate, keeps "default" as the device and follows it, and
reopens on rate changes. Checked on BlackHole (48 kHz kept, a change to
96 kHz followed) and with Getting Started at 48 kHz (passed, 10 221
callbacks at 21.33 ms, load ≤ 31 %, the mixer's threads in the device's
AudioWorkGroup). Measured through BlackHole as a loopback (30 isolated key
taps, 48 kHz, 1024 frames, on AC power, each onset in the recording timed
against the trace's stamps): from the start of the block a note is rendered
in to its sound, 44.1 ms (p50) with the AudioQueue, 22.8 ms direct: the
queued block (21.3 ms) gone; key to sound 55.6 → 34.0 ms (p50), 62.4 →
42.6 ms (p90). Not checked: a switch of the system's default device while
playing.

## Learned on macOS

- **A default set in `onPreInit` does reach the driver.** All modules'
  `onPreInit` run after the settings are loaded (`GlobalModule::onPreInit`)
  and before any `onInit`. `Settings::setDefaultValue` on a key with no item
  yet makes the value too; on an existing one it changes only the default.
  So Orchestrion's default set there is the value the driver is opened with,
  `AudioConfiguration::init`'s becomes the default, which is what
  `switchToDefaultAudioDriver` falls back on, and a stored value still wins.
  The trace's `meta.txt` shows it (`driver=CoreAudioDirect (OSX)`). The
  bullet below holds for `onInit`, not `onPreInit`; PipeWire could take the
  same route (`PipeWire` as the value, MuseScore's `ALSA` left as the
  fallback).

## Learned on Linux

- **A default set by another module comes too late, or is overwritten.**
  `AudioModule::onInit` calls `AudioConfiguration::init` (which sets the
  `audio/io/*` defaults) and opens the driver (`startAudioProcessing`) in the
  same call: an Orchestrion module's `onInit` runs before it (its default is
  then overwritten) or after it (the driver is already open). What can be
  changed is what the driver gets: the factory registered as
  `IAudioDriverFactory` (Orchestrion re-registers it, wrapped, in
  `OrchestrionSynthesisModule::registerExports`), and through it the driver
  and the spec it is opened with (driver name, rate, buffer size).
- **A stream whose rate isn't the device's costs the alignment buffer.**
  PipeWire's client opened at 44.1 kHz on a 48 kHz graph gets 940/941 frames
  per callback, fewer than the engine's 1024-frame block: renders skip
  callbacks (`useAlignBuffer`), as on Windows. Same on any driver that
  resamples: open at the device's rate.
- **Done on Linux** with `DeviceFollowingAudioDriver`: `outputDevice::
  sampleRate` is the PipeWire graph's (`PipeWireGraphRate`); and `PipeWire`
  is now the driver's default the macOS way, in `onPreInit`.

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
