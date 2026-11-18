# Handover: lower latency on Windows

For an agent working on Windows, on branch `latency-lowering`. Delete this
file once its tasks are done (or fold what's still true into
`src/OrchestrionSynthesis/gestureToSound.md`). A macOS counterpart,
`latency-macos-handover.md`, runs in parallel: read its "Where things stand"
for the branch's state and the tracer (`--latency-trace <dir>`,
`tools/latency_trace.py`), and its task 2 for the thread-priority helper you
share.

## What a trace on Windows showed

Windows 11 25H2, x86_64, driver `WASAPI` (shared mode), asked for 44.1 kHz
and 1024 frames, opened at 48 kHz (the mix format's: no resampling):

- **The device calls back every 10 ms, with 480 frames** (its shared-mode
  period), not 1024. The engine still renders 1024 frames at a time, through
  the alignment buffer of the callback
  (`MuseScore/muse/framework/audio/main/internal/startaudiocontroller.cpp`,
  `useAlignBuffer` when the driver asks for less than the engine's block):
  it renders on every second or third callback (`.R.R.R..R.R…`), 20 or
  30 ms apart.
- So a note waits up to **30 ms** for the next render (gesture note-on to its
  block: 10.2 ms p50, 19.1 p90, 30.0 max), where one period, 10 ms, would do.
- And each render (1.7 to 2.8 ms) falls into one 10 ms period: **load up to
  89 %** of it (31 % p90, 43 % p99), with FluidSynth only. A VST, or a slower
  machine, and it underruns.
- **Hidden latency**: `WasapiAudioDriver::th_audioInitialize` asks for a
  buffer of the engine's block (1024 frames, 21.3 ms; it got ~1056), and
  fills what's free of it at each event: what it writes sits behind
  ~576 frames (12 ms) already queued, which the trace can't see.
- The automatic player fires its notes 1.0 ms late p50, 3.2 max (Linux 0.17,
  1.1): timer resolution and thread priority.
- **The WASAPI thread has no MMCSS registration** (no
  `AvSetMmThreadCharacteristics` in the driver).

## Where things stand (2026-11-18)

Traced with Getting Started (`--test-case GettingStarted --latency-trace`),
this machine's default device now at 44.1 kHz (period 441 frames):

- **Done: 256 frames by default** (`OrchestrionSynthesisModule::onPreInit`,
  `Q_OS_WIN`). Every callback renders its 441 frames; note-on to its block
  5.0 ms p50, 10.0 p99 (was 10.2 / 30.0 max); load 4 % p50.
- **Done, unverified: low-latency shared mode** (muse commit `33c6c74cf`,
  local branch `orchestrion-latency-wasapi` in `MuseScore/muse`, not pushed,
  gitlinks not bumped). This device offers no shorter period
  (`GetSharedModeEnginePeriod`: default = fundamental = min = max = 441), so
  `InitializeSharedAudioStream` wasn't run; its fallback to `Initialize` on
  the same client after a failure is untested too. Needs a device with the
  inbox HD Audio or USB Audio 2 class driver.
- **Done: MMCSS "Pro Audio"** for the WASAPI thread (same commit):
  processing max 3.3 → 1.7 ms, load max 33 → 17 %.
- **Done: the margin queued, chosen** (muse `IAudioOutputMonitor`,
  Orchestrion `IOutputLatency`, Audio/MIDI > Latency…). Filling WASAPI's
  buffer (970 frames, ~2.2 periods, whatever is asked) put each write behind
  ~529 frames: 31.7 ms from a callback to its first frame played
  (`IAudioClock`). The driver now keeps one, two or three whole periods
  queued (Low, Medium, Large; half a period more buys no time, a pass takes
  a whole one), in a buffer of three periods: one period measured 19.7 ms.
  It detects underruns (a write after the pass that needed it, from the
  event grid and the queue after the last write; and the stream's clock
  stalling), and reports headroom and load. Automatic (the default) raises
  the level after an underrun or under 1 ms of headroom, lowers it after
  5 minutes of sound with a period to spare (doubled after each raise),
  only while silent. Checked: a callback made 15 ms late is an underrun at
  Low (heard, and the clock stalled 9.8 ms), the level rose, and later ones
  weren't. Open: whether the system's own glitches stall the stream's clock
  (the dialog's "Silence played by the device" says so); the other
  platforms' monitors (macOS: the IOProc's buffer size, Linux: PipeWire's
  `node.latency`).
- **Timed threads above normal** (`raiseTimedThreadPriority`): no
  measurable effect, see `threading.md`, "What's left".

## Task 1: render every period, with a small WASAPI buffer

The engine's block size (`audio/io/bufferSize`, MuseScore's
`AudioConfiguration`) is used both as the engine's block and as the WASAPI
buffer duration. The callback renders straight into the driver's buffer
when the driver asks for at least the engine's block (no alignment), so:

1. **Try the smallest offered size** (`availableBufferSizes`: 256 here):
   the engine should then render the 480 frames the driver asks for at
   every callback, and WASAPI's buffer shrink to its shared-mode minimum.
   Check in a trace: every callback renders (processing > 0 on each),
   load flat, note-on to block at most one period. Find out what WASAPI
   does with a buffer duration below its period (log `m_bufferFrames`,
   `m_defaultPeriod`, `m_minimumPeriod` after `Initialize`, e.g. a `LOGI` in
   the driver, or add them to the trace's `meta.txt` through the driver).
   Orchestrion can set the default without forking: `muse::settings()->
   setDefaultValue` on `Settings::Key("audio", "io/bufferSize")` in
   `OrchestrionSynthesisModule::onPreInit`, as is done there for macOS'
   driver key (in `onInit` it would come too late: see the macOS handover's
   "Learned on macOS"), under `#ifdef Q_OS_WIN` until the other platforms
   settle theirs. Or through the driver: `DeviceFollowingAudioDriver` (on
   the branch, wrapped around every platform's driver) already rewrites the
   spec's rate in `open`; the buffer size could go there too (the engine is
   set up from the active spec the driver returns).
2. **Low-latency shared mode** (the bigger gain, a patch to the muse fork's
   WASAPI driver): `IAudioClient3::GetSharedModeEnginePeriod` and
   `InitializeSharedAudioStream` at the engine's minimum period (often 128
   to 144 frames at 48 kHz, ~3 ms) when the engine block asks for less than
   the default period, falling back to `Initialize` where unsupported. Fork
   patches go on the muse framework's `orchestrion-<date>` branch, see
   `CLAUDE.md`, "Fork patches".

## Task 2: priorities

- **MMCSS for the WASAPI thread** (`th_audioThread`):
  `AvSetMmThreadCharacteristicsW(L"Pro Audio", &index)` once it starts,
  `AvRevertMmThreadCharacteristics` as it ends (Avrt.lib). A fork patch too.
- **Orchestrion's timed threads**: the Windows branch of the helper planned
  in the macOS handover's task 2 (`THREAD_PRIORITY_ABOVE_NORMAL`, or MMCSS
  "Games"/"Playback" for the play and output threads). If the helper isn't on
  the branch yet, create it as described there (in `OrchestrionCommon`,
  called by each thread as it starts), with the Windows branch only.
  Verify: `auto note-on`'s `played` step.

## Practicalities

- Same as the macOS handover: build per `CLAUDE.md`, commit on
  `latency-lowering`, `clang-format --dry-run --Werror` every touched
  `.h/.cpp`, traces' before/after numbers in the commit messages.
- Same session shape for every trace: a minute of keys, a minute of
  automatic play, quit normally. `tools/latency_trace.py` needs Python 3
  only.
