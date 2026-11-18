# Handover: the latency dialog on Linux, and its translations

For an agent working on Linux, on branch `latency-lowering`. Delete this file
once its tasks are done (or fold what's still true into
`src/OrchestrionSynthesis/gestureToSound.md`). Read
`latency-windows-handover.md`'s "Where things stand" first: it tells how
the Windows side came about, with its measurements.

## What exists (from Windows)

- **`muse::audio::IAudioOutputMonitor`** (muse fork,
  `framework/audio/iaudiooutputmonitor.h`): optional, implemented by a
  platform driver class and found by `dynamic_pointer_cast` from the
  driver. It reports windows (`takeWindow`: underruns, clock stalls,
  headroom mean/least, load mean/most, output peak and latency), the
  device's period, and up to three **margin levels** (least first, in ms),
  set with `setMarginLevel` only while nothing sounds;
  `delayNextCallback` makes a callback late, to try the detection.
  Only `WasapiAudioDriver` implements it (`th_processAudioData`,
  `th_measureClock`, read them as the reference).
- **`dgk::IOutputLatency`** (`OrchestrionSynthesis/IOutputLatency.h`,
  `internal/OutputLatency`): OS-independent. `DriverCapturingFactory`,
  innermost in the factory chain (`OrchestrionSynthesisModule::
  registerExports`), hands it each driver made; it polls the monitor every
  250 ms on the main thread, keeps 10 s of windows and 2 min of seconds,
  and runs the automatic setting (off by default: Low, the user's, with
  an "Audio glitch?" toast after an audible underrun): up a level after an
  underrun or under 1 ms of headroom, down after 5 minutes of sound with a
  period of headroom to spare (doubled after each raise in the session),
  reacting only to what happens while the output sounds, and applied once
  the app isn't playing (automatic player stopped, no key or pedal held,
  output silent 500 ms: peak under -80 dBFS). So **a monitor must report
  the output's peak**, or nothing ever counts as sounding: no raise, and
  the level never comes down.
- **Audio/MIDI > Latency…** (`src/qml/LatencyDialog.qml`,
  `view/OutputLatencyModel`): Automatic (with a ringed ? popup), Low /
  Medium / Large, headroom, load, underruns, "silence played by the
  device" (clock stalls), a 2-minute strip, Start over, and a button making
  a callback 15 ms late. Without a monitor it says it isn't available.

## Task 1: translations: done

Done on 2026-11-18: the `lupdate` target didn't know `qsTrc` (MuseScore's
QML function with a context, which the dialog uses): aliased to
`qsTranslate`. The alias also brings MuseScore's QML strings into the `.ts`
files, left untranslated (English, as before). German and French checked in
the dialog under Xvfb; French's menu item is `La&tence…` (L is the playback
device's).

The task was:

The dialog's strings (context `orchestrion/latency`) and the menu item
`&Latency…` (context `appshell/menu/audio-midi`, in
`src/MuseScoreShell/view/OrchestrionMenuModel.cpp`), and the "Audio
glitch?" toast's (`src/qml/GlitchToast.qml`, `qsTr`) are in neither
`share/locale/*.ts`: run the `lupdate` target, translate German and French,
check them in the dialog (the popup's text wraps at 280 px; the level
buttons share the dialog's width three ways).

## Task 2: a monitor for PipeWire (and ALSA): done for PipeWire

Done on 2026-11-18 (muse branch `orchestrion-latency-pipewire`, on top of
the WASAPI one): a level is the quantum asked for (`node.latency`, updated
on the running stream), the engine's block (256 frames by default on Linux
too, `OrchestrionSynthesisModule::onPreInit`) times 1, 2, 4: 5.3, 10.7,
21.3 ms at 48 kHz. Headroom: to the start of the next cycle (the stream
isn't processed in PipeWire's realtime thread, so a callback's frames play
from the next cycle). Underruns: the graph's clock (`pw_time.ticks`) goes on
while the frames taken from the stream don't. `command://orchestrion/latency`
(`level`, `delay`; answers with the status) drives it from a script.
Checked on a null sink, a callback made late while playing: at Low, 8 ms
late starves 5.3 ms, 15 → 10.7, 30 → 26.7; at Medium 8 ms passes; at Large
15 ms passes. Left: no clock stalls reported (the dialog's orange cells never
show here); ALSA has no monitor. A level change makes the graph starve
the stream for a cycle or so: the 20 callbacks after one aren't counted. On this ThinkPad, underruns
of ~200 ms every 10.75 s are the platform's (see the dGPU's
`power/control`), not the app's.

The plan it followed:

Implement `IAudioOutputMonitor` in the muse fork's PipeWire driver (and
ALSA's if cheap), so the dialog works here. Things to decide:

- **What a level is.** On Windows a level is how much is kept queued in
  WASAPI's buffer (whole periods: a pass of Windows' engine takes one).
  PipeWire has no such queue on our side: the margin is the quantum asked
  for (`node.latency`), and the engine's block should follow it (a quantum
  smaller than the block brings back the alignment buffer: renders on some
  callbacks only). The muse way to change both is
  `IAudioDriverController::changeBufferSize`, which reopens the driver and
  updates the engine's spec: a driver can't call it itself. Options: let
  `setMarginLevel` reconnect the stream with the new latency (if the engine
  copes with a block change without the controller), or have the monitor
  say its levels are block sizes and let `OutputLatency` apply them
  through the controller. Either way, only while silent (already so).
- **Underruns.** Check what PipeWire tells a stream: whether
  `pw_stream_get_time_n` or the driver node's clock (`spa_io_clock`, an
  `xrun` count in recent versions?) counts them, else infer from the
  process callback's timing against `pw_time.now` / the quantum, as WASAPI
  does from its events' grid. ALSA: a write returning `-EPIPE`.
- **Headroom**: from the callback's end to the moment the graph needs the
  buffer (the driver's next cycle), in ms.
- Try it with the dialog's test button: at the least level a callback
  15 ms late must count as an underrun and be heard (on Windows it was:
  written 8.5 ms after it was needed, the clock stalled 9.8 ms), and the
  automatic setting must raise the level, after which the same delay
  passes.

## Practicalities

- Muse patches go on the muse fork's `orchestrion-latency-wasapi` branch
  (or a new one on top), then a MuseScore commit bumping it (branch
  `testflow`), then the gitlink here: see `CLAUDE.md`, "Fork patches".
- `clang-format --dry-run --Werror` every touched Orchestrion `.h/.cpp`;
  measurements before/after in the commit messages.
