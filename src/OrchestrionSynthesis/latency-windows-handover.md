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
   Orchestrion can't set it with `muse::settings()->setDefaultValue`: the
   defaults are set and the driver opened in the same call
   (`AudioModule::onInit`, see the macOS handover's "Learned on Linux"). Do it
   through the driver instead: a decorator whose `open(spec, activeSpec)`
   lowers `spec.output.samplesPerChannel` before passing it on (the engine is
   set up from the active spec the driver returns), made by a factory
   decorator wrapped in `OrchestrionSynthesisModule::registerExports` like
   `PipeWireFirstAudioDriverFactory` (under `#ifdef Q_OS_WIN` until the other
   platforms settle theirs). The macOS handover plans a driver decorator for
   the device's rate: if it's on the branch, add the buffer size there.
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
