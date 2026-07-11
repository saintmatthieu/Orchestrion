---
name: verify
description: Build, launch, and drive Orchestrion headlessly to verify changes end-to-end. Use when a change needs runtime verification (GUI behavior, playback, sequencer logic).
---

# Verifying Orchestrion changes at runtime

## Build

```bash
cmake --build build/Debug --target install   # binary: build/Debug/install/bin/Orchestrion
```

## Launch headless (VNC) and drive with vncdotool

The app links Qt from `~/Qt/<version>/gcc_64`, which ships the `vnc` platform plugin.
`pip install vncdotool` provides `vncdo` for screenshots + synthetic input.

```bash
LANG=fr_FR.UTF-8 LC_ALL=fr_FR.UTF-8 LANGUAGE=fr \
QT_QPA_PLATFORM="vnc:size=1600x900" \
build/Debug/install/bin/Orchestrion > /tmp/orchestrion.log 2>&1 &   # serves VNC on :5900

vncdo -s 127.0.0.1::5900 move 500 150 pause 0.2 click 1 capture out.png
vncdo -s 127.0.0.1::5900 key ctrl-l          # shortcuts work
vncdo -s 127.0.0.1::5900 key 6               # computer-keyboard gesture: play a right-hand note
```

Gesture keys: `1`–`5` = left-hand pitches (0,1,3,4,5), `6`–`0` = right-hand (60–64).

Playback has TWO distinct drivers — exercise both when verifying sequencer
changes: gesture keys (above) and automatic playback (`key space`), which runs
through `AutomaticOrchestrionPlayer` and has its own re-entrancy/timing paths
(a loop-wrap stack overflow once hid from gesture-only testing).

## Gotchas (all bitten in practice)

- **Locale assert**: `share/locale/languages.json` lists only `de`/`fr`. With an
  `en_US` system locale a Debug build hits an assert in
  `LanguagesService::loadLanguage` at startup. Launch with a French locale (above).
- **No positional score arg in Debug**: passing a score path on the command line
  trips `assert(m_startupProjectFile.url.isEmpty())` in
  `OrchestrionStartupScenario::init`. Instead set the startup score via
  `paths\lastOpenedProjectsPath` in
  `~/.config/Unknown Organization/OrchestrionDevelopment.ini` — **back it up and
  restore it after** (the app rewrites it on graceful exit; the user's own
  running instance also shares it).
- **Multi-instance forwarding**: if the score you open is already open in another
  running Orchestrion instance (the user often has one running), the open request
  is forwarded over IPC to that instance and your instance shows nothing. Pick a
  score no other instance has open.
- **Don't kill the user's instance**: target yours precisely, e.g.
  `pkill -f "Orchestrion2/build/Debug/install/bin/Orchestrion"`.
- **Controls overlay fades after 2 s** and hovering the buttons does NOT count as
  mouse activity. Move over the notation area first, then move+click the button
  within the window (small `pause` values, < 2 s total).
- **Icons are invisible on VNC**: the playback buttons render via
  `ColorOverlay` (shader effect) which needs GPU; on the software VNC platform
  only plain Items (rectangles, score canvas, tooltips) render. Buttons are still
  clickable at their computed positions. With the default 800x350 window the
  playback row sits at y≈56, buttons at x≈514,552,590,628,666,704 (loop),
  fullscreen ≈756.
- **VNC captures swap red/blue**: `vncdo capture` PNGs come out BGR (the app's
  warm cream/espresso theme shows as cool blue). Geometry is trustworthy;
  before judging COLORS, swap channels:
  `python3 -c "from PIL import Image; i=Image.open('x.png').convert('RGB'); r,g,b=i.split(); Image.merge('RGB',(b,g,r)).save('x_true.png')"`
- Logs also land in `~/.local/share/OrchestrionDevelopment/logs/`.
