# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Orchestrion is a Qt 6 / C++20 desktop application built on top of a forked MuseScore. It reuses MuseScore's modular framework (the `muse::modularity` IoC container, the `IApplication` lifecycle, and most of the `mu::` / `muse::` modules — engraving, notation, project, playback, audio, vst, etc.) and adds Orchestrion-specific modules that turn the score into something the user can "play" via gesture controllers / external MIDI devices.

The MuseScore fork lives in the `MuseScore/` git submodule (https://github.com/saintmatthieu/MuseScore.git, branch `orchestrion-<date>`, a handful of Orchestrion commits on top of upstream `main` — MuseScore 5.0-dev). Treat it as upstream code — the build glue (`MuseScore.cmake`) is adapted from upstream's root `CMakeLists.txt`. MuseScore itself has two nested submodules: `MuseScore/muse` (the Muse framework, https://github.com/musescore/muse_framework — `MUSE_FRAMEWORK_PATH`) and `MuseScore/muse_deps` (recipes and a lock file for prebuilt third-party libraries, downloaded at configure time). `DSPFilters/` is the other top-level submodule. **Always clone with `--recurse-submodules`** (or run `git submodule update --init --recursive`); a missing submodule manifests as a `FATAL_ERROR` about nested submodules or confusing missing-target errors during CMake configuration.

Git LFS is used for `*.jpg` (see `.gitattributes`) — run `git lfs pull` after a fresh clone.

## Build

CMake 3.28+ (3.31+ on macOS) + Ninja, Qt 6.10+ (CI: 6.10.2; local Macs: 6.10.x — upstream declares 6.8 but its framework uses 6.10 API), C++20 (upstream wants AppleClang 21 / GCC 14 / MSVC 19.40; AppleClang 17 works, the version check only warns). Allman braces (`.clang-format`) for Orchestrion code.

Third-party dependencies (zlib, freetype, harfbuzz, libpng, the VST3 SDK sources, ...) come from `muse_deps`: `MuseScore.cmake` includes `muse_deps/buildtools/manifest.cmake` and the framework's `ExtDepsManifest.cmake`, which download prebuilt archives into `<build>/_deps` at configure time and install the shared ones into the bundle (`extdeps_install_consumed`). No libsndfile is needed anymore (FluidSynth decodes SF3 with the vendored vorbis decoder).

```bash
# Configure (one-time; pick a build dir per config)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_PREFIX_PATH=~/Qt/<version>/<platform>

# Build + install (install is required to lay out the runtime tree the launcher expects)
cmake --build build --target install
```

`CMAKE_INSTALL_PREFIX` is forced to `${CMAKE_BINARY_DIR}/install` on Linux/Windows and to the binary dir itself on macOS — do not override it. The `.vscode/launch.json` `preLaunchTask` is `CMake: install`, not just build, for this reason.

The Linux executable lives at `build/install/bin/Orchestrion`. The un-installed `build/bin/Orchestrion` is runnable too: the app resolves resources as `<exe dir>/../share/`, and configure creates a `build/share -> build/install/share` symlink, so it works once install has run at least once (`.vscode/launch.json` relies on this to debug whatever CMake Tools launch target is selected — including test binaries, which aren't installed). On Linux the binary is placed in a `bin/` subdirectory of the build tree on purpose: the Qt QML module creates an `Orchestrion/` directory that would otherwise collide with the extension-less executable name (see comment in top-level `CMakeLists.txt`).

CI mirrors this with `ci_build.cmake` (configures + builds + installs under `build.release`), invoked from `ci_build.bat` on Windows. The GitHub workflow (`.github/workflows/orchestrion.yml`) builds Windows / macOS / Linux and packages NSIS / DMG / AppImage respectively.

Unit tests are off by default (`MUSE_ENABLE_UNIT_TESTS` defaults to OFF in `MuseScore.cmake`); configure with `-DMUSE_ENABLE_UNIT_TESTS=ON` to get the Orchestrion test targets (`orchestrionsequencer_tests`, `positionestimation_tests`). CI builds none.

## Runtime layout / data files

The install tree contains soundfonts, scores, wallpapers, icons, the prebuilt dependency libraries, and Qt's `translations/` tree. The locations differ per platform (`MUSE_APP_INSTALL_RESOURCES_LOCATION` in `MuseScore.cmake`, which MuseScore's `share/` rules use, and `resource_install_dir` in the root `CMakeLists.txt`) — if you touch resource installation, check all three branches.

Translations are produced from `share/locale/*.ts` via `qt_add_lrelease`; the install step also copies Qt's bundled `qt_*.qm` (excluding `qt_help_*` and `qtbase_*`). `share/locale/overrides/muse_overrides_<lang>.ts` (English included) reword the MuseScore strings Orchestrion can show ("…MuseScore Studio…"): the `lupdate` target doesn't scan `MuseScore/src`, so they are edited by hand, and every language in `languages.json` needs one (`languages.cfg`'s `resource_names`).

`testscores/` holds MusicXML scores for testing specific features by hand (e.g. the ornament scores, generated by `testscores/gen_ornament_scores.py`). It is deliberately **not** installed: the bundled `scores/` folder is listed in the score shelf's Library of release builds.

## Architecture

### Modular IoC, MuseScore-style

MuseScore's framework lives in `MuseScore/muse/framework/<module>` (`global`, `ui`, `uicomponents`, `audio`, `vst`, `midi`, `interactive`, `multiwindows`, ...; include as `<module>/header.h` or `framework/<module>/header.h`) and MuseScore's own modules in `MuseScore/src/<module>` (`engraving`, `notation`, `notationscene`, `playback`, `project`, `appshell`, ...). QML-backed view classes now live inside the modules' QML directories, e.g. `notationscene/qml/MuseScore/NotationScene/notationpaintview.h` and `appshell/qml/MuseScore/AppShell/mainwindowtitleprovider.h`. `MuseScore.cmake` lists which MuseScore/framework modules are built (`MUE_BUILD_*` / `MUSE_MODULE_*` as plain `set()`s — they are a fixed configuration; stubs are used for the rest).

The framework has a **global IoC** (`globalIoc()`, `GlobalInject<I>`, interfaces declared `MODULE_GLOBAL_INTERFACE`) and **per-window context IoCs** (`ioc(ctx)`, `ContextInject<I>`, `MODULE_CONTEXT_INTERFACE`). Orchestrion is a single-window app: `src/OrchestrionCommon/OrchestrionIoc.h` records the one context (`dgk::iocContext()`), and Orchestrion classes derive from `dgk::Injectable` and declare services as `dgk::Inject<I> name{this};` — it picks `GlobalInject` or `ContextInject` from the interface's declaration (a context miss falls back to the global IoC). Orchestrion's own interfaces are `MODULE_GLOBAL_EXPORT_INTERFACE` and are registered in `globalIoc()` from `registerExports()`. Anything that touches MuseScore's context-scoped services (`IProjectSession` — formerly `IGlobalContext`, `IPlaybackController`, `IPlayback`, `IUiActionsRegister`, ...) runs in the **context phase**: each Orchestrion module implements `newContext(ctx)` returning a `dgk::ModuleContextSetup` whose hooks (`onInit`, `onAllInited`, `resolveImports`) forward to the module's `onContext*` methods.

Each top-level directory in `src/` (except `App`, `OrchestrionCommon`, `qml`, `stubs`) is a **module** following MuseScore's framework conventions:

- `<Module>Module.{h,cpp}` — module entry point (registers services, types, QML imports).
- `I<Service>.h` at the module root — public interfaces consumed via the IoC container.
- `internal/` — implementations, registered with the container by the module's `registerExports`.
- `view/` — QML-backed C++ view-models (`QObject` subclasses).
- `qml/` — module-local QML imported via `MODULE_QML_IMPORT`.
- `<Module>.qrc` — Qt resources (icons, QML, etc.).

`CMakeLists.txt` files use the framework's legacy module macros (`muse/buildscripts/cmake/DeclareModuleSetup.cmake`): `declare_module(...)`, populate `MODULE_SRC` / `MODULE_LINK` / `MODULE_QML_IMPORT` / `MODULE_QRC`, optionally `set(MODULE_USE_PCH OFF)` / `set(MODULE_USE_UNITY OFF)`, then `setup_module()`. Qt targets are linked for all Orchestrion targets by `link_libraries(${QT_LIBRARIES})` in `MuseScore.cmake`. Follow the same pattern for new modules.

The Orchestrion modules (registered in `OrchestrionAppFactory::newGuiApp`):

| Module | Responsibility |
| --- | --- |
| `MuseScoreShell` | Hosts MuseScore's notation shell, action ids, top-level shell types |
| `OrchestrionShell` | Orchestrion-specific app shell + startup scenario, top-level QML |
| `OrchestrionNotation` | Notation interaction overrides specific to Orchestrion playback |
| `OrchestrionOnboarding` | First-run / onboarding views |
| `OrchestrionSequencer` | Core "play the score" engine: chord/melody/voice segmentation, automatic player, modifiable-item registry |
| `OrchestrionSynthesis` | Wires per-track VST synthesisers into MuseScore's mixer (see `src/OrchestrionSynthesis/classDiagram.md`) |
| `ScoreAnimation` | Visual feedback synced to playback (segment registry, animator) |
| `ExternalDevices` | MIDI / audio device discovery and connection |
| `GestureControllers` | Computer-keyboard and MIDI-device gesture controllers, always on, merged into one note-event stream (`IGestureInput`) |
| `OrchestrionConfiguration` | App-wide configuration / preferences |

The factory lives in `src/App/OrchestrionAppFactory.cpp` — when adding a new module, register it there alongside the existing `app->addModule(...)` calls and add it to the top-level `CMakeLists.txt` (`add_subdirectory` + `target_link_libraries`). Framework modules that Orchestrion doesn't build are added through their stubs; include the real or stub header under `#ifdef MUSE_MODULE_<X>` (from `muse_framework_config.h`) — mixing a stub header with a real module (or vice versa) crashes at startup.

`OrchestrionApp` (`src/App`) derives from the framework's `muse::ui::GuiApplication`: `main.cpp` calls `setup()` (module registration and init, global phase) then `setupNewContext()` (context phase, which loads the main window from `mainWindowQmlPath()` — `qrc:/qt/qml/Orchestrion/src/qml/Main.qml`, the compiled `Orchestrion` QML module — and runs `doStartupScenario()`). `CommandOptions` derives from `muse::CmdOptions`.

### Audio

MuseScore's audio engine runs on its own thread and talks to the main thread over an RPC channel (`audio/main` = app side, `audio/engine` = engine side). `OrchestrionSynthesis` registers `OrchestrionSynthResolver` for the built-in (`Fluid`) source type once the audio has started and routes the score's tracks to it with `IPlayback::setSourceParams`; the wanted synthesizer (built-in FluidSynth, a VST, or none) is carried in the params' `resourceMeta.attributes`, so changing it changes the params and makes the engine re-resolve the track's synthesizer (identical params are a no-op). `OrchestrionSynthesizerWrapper` implements `ISynthesizer` and is fed by the gesture sequencer, not by the score's playback data; the engine processes it while idle too. Replacing the built-in resolver destroys upstream's `FluidResolver`, whose destructor also deletes the process-wide Vorbis decoder the vendored FluidSynth needs for SF3 soundfonts — Orchestrion's `FluidSynthesizer` installs its own (`ensureFluidSynthSupport`) and routes FluidSynth's log into the app log; without it every sample fails to load and playback is silent (FluidSynth only complains on stderr).

### Run modes

`main.cpp` branches on `CommandLineParser::runMode()`:
- `GuiApp` (default) — full `QApplication` + UI.
- `AudioPluginRegistration` — headless `QCoreApplication` for VST scanning.

The console-app construction path is `OrchestrionAppFactory::newConsoleApp` (don't lump it together with the GUI path).

### Testflow (scripted runs)

MuseScore's testflow module runs JavaScript scenarios in the GUI app: `Orchestrion --test-case <script or name> [--test-case-speed Fast]` (`src/App/TestflowRunner`, adapted from Audacity's) runs the script once the window is up and exits with 0 (passed), 1 (failed) or 2 (couldn't run). Scripts live in `testflowscripts/` (installed, see the tutorials below); `require("steps/...")` and script names resolve against `MUSE_TESTFLOW_SCRIPTS_PATH`, reports go to `MUSE_TESTFLOW_DATA_PATH`/reports. Scripts drive the app with commands through `api.commands.dispatch(uri, params)` (a muse fork patch), which returns `{success, code, text}` once the command has responded — no guessed sleeps. Orchestrion's commands (`command://orchestrion/...`, incl. `open-score?path=`) are registered in `OrchestrionShell/internal/OrchestrionCommands.cpp`; their handlers run from a queued Qt event, because commands are dispatched while kors' queues are being processed, which holds the main thread's queue lock — a handler joining a thread that sends to the main one (opening a score rebuilds the sequencer) deadlocks there. Run headless with Xvfb and a paced silent audio output (see the verify skill: ALSA's `null` device starves the audio engine): `tools/run_testflow.sh <app or .AppImage> <script> [--record video.mp4]` sets all that up (fresh profile, Xvfb + openbox, a null sink, ffmpeg) and exits with the script's result; a script's leading comments may hold `@testflow record-from <step name>` / `record-after <step name>` to start its video at the start / end of that step (the runner prints each step's wall-clock start/end, and the video is cut there), and `@testflow test` for a script that isn't a tutorial (the others are). Recording, the runner starts the app with `--full-screen`: scripts leave the window as it is, since tutorials also run in the user's session. CI's `testflow_linux` job runs every script against the AppImage that way and uploads the videos. The scripts are installed (`share/testflowscripts`) and those not marked `@testflow test` are **tutorials** (`ITutorials`, `OrchestrionShell/internal/Tutorials`): Help ▸ Tutorials… (F1; Cmd+? on macOS) opens the score shelf showing them only, and the score shelf lists them first; one runs in the user's session, on the open score (Frère Jacques if none is), Esc stops it, and the app stays open. For demos, scripts click controls as a user would with the **demo cursor** (`OrchestrionShell/internal/DemoCursor`, drawn by `src/qml/DemoCursorOverlay.qml`): `command://orchestrion/demo-click?control=<section/panel/control>` glides a drawn pointer to the control's navigation path and sends real mouse events (moves, press, release) to the window, answering once released; `demo-keys`, `demo-caption` and `demo-key-press`/`-release` show the number keys and a caption and press keys with real key events (`DemoKeys`, drawn by `src/qml/DemoKeysOverlay.qml`), as `testflowscripts/GettingStarted.js` does. The app's own pointer handling must therefore go by the events' positions, not `QCursor::pos()`, which the demo cursor doesn't move. Main window only.

### QML / UI

Top-level QML is registered via `qt_add_qml_module(Orchestrion URI Orchestrion ...)` in the root `CMakeLists.txt`; every file in `src/qml/` must be listed there (`QML_FILES`) as well as in `resources.qrc`. Module-internal QML lives under each module's `qml/` and is imported via `MODULE_QML_IMPORT` from that module's `CMakeLists.txt`. See `qml-architecture.md` for the high-level shell composition (`AppWindow → WindowContent → {HomePage, NotationPage, DockToolBar, ...}`).

MuseScore's QML is made of compiled QML modules (`Muse.Ui`, `Muse.UiComponents`, `Muse.Interactive`, `MuseScore.AppShell`, `MuseScore.NotationScene`, ...): import them **without a version** (a versioned import of a module that ships scripts, such as `Muse.UiComponents`, is "ambiguous" against the unversioned dependency import another module pulls in), and in files that also use `QtQuick.Controls` import `Muse.UiComponents` (and modules depending on it) **qualified** — its `MenuItem` type would otherwise shadow the Controls one. Types can't be injected into those modules with `qmlRegisterType`; Orchestrion registers its own types under `Orchestrion.*` URIs (`OrchestrionMenuModel`, `MainWindowBridge` in `Orchestrion.MuseScoreShell`). The Windows/Linux window chrome (`OrchestrionTitleBar`, `OrchestrionMenuBar`, `OrchestrionSystemButtons`) and the macOS menu bar (`MacMenuBar`) are Orchestrion copies of MuseScore's, using `OrchestrionMenuModel`.

## Conventions to respect

- **Namespace**: project-local code is in `namespace dgk` (see `OrchestrionAppFactory`, `CommandLineParser`, etc.). MuseScore code uses `mu::` / `muse::`.
- **Application name** is intentionally *not* `"Orchestrion"` for stable builds in some places — `main.cpp` sets it to `"OrchestrionDevelopment"` under `MUSE_APP_UNSTABLE`. The comment there explains why (changing it would lose user settings); preserve that.
- **GCC needs `-include cstring`** added globally (top of root `CMakeLists.txt`) because some MuseScore sources use `memset`/`memcpy` without including the header. Keep it.
- **Fork patches to MuseScore** are a short series of commits on the submodule's `orchestrion-<date>` branch (engraving: `unrollRepeatsInPlace`, layout tick warp, `UndoStack::markClean`; notation: `OrchestrionNotationInteraction`, painting; notationscene: paint hooks; musicxml: inferred cresc./dim. lines; project: null guard for the cloud stub in `ConvertFileToScoreService`; `version.cmake`; `SetupConfigure.cmake`). The `muse` framework submodule is patched too: a few commits (VST parameter routing, fx chain order, audio plugin registration fixes) on branch `orchestrion-<date>` of https://github.com/saintmatthieu/muse_framework, based on the commit upstream MuseScore pins. When bumping MuseScore again, rebase or re-apply both series (muse first, onto the commit the new MuseScore pins), and update the MuseScore commits that bump the `muse` gitlink.
- **kors_async channels copy-assign their payload**: event value types sent through `muse::async::Channel` must be assignable (no `const` members).
- **kors_async subscriptions are `SetOnce` by default and assert on a duplicate in debug builds** (`callback is already set`). A handler that re-subscribes the same receiver to an object that may be unchanged (e.g. the modifiable-item registry or the player on `sequencerChanged`) must pass `muse::async::Asyncable::Mode::SetReplace`; a subscription that is meant to be made once must not be repeated (the old kors_async silently kept the first one, which some code relied on).
- **Closing a score** (opening another one, window close, Cmd+Q) goes through `OrchestrionActionController::closeCurrentProject`, which asks about Orchestrion's own modifications and closes the project itself. Never route through MuseScore's `closeOpenedProject()`: since MuseScore 5 every imported (MusicXML) score counts as "needing a save" (`NotationProject::isNeedSave` is a flag set on import, not derived from the undo stack), so it would always ask to save.
- License headers: files carry a GPL-3.0-or-later header (`Copyright (C) <year> Matthieu Hodgkinson`). New files get the same header with the **current year** (existing files keep the year they were written in; don't bump them).
- **Transport glyphs are generated**: `icons/player/{rewind,previous,play,stop,next,loop,fullscreen}.svg` are cut from the title typeface by `tools/gen_player_glyphs.py` (needs fontTools; run from the repo root). Don't hand-edit them — change a constant in the script and regenerate. The buttons themselves are `TransportButton`, which glows rather than swapping glyphs, so there is no pause icon.
- **Shortcuts are bound to commands, not actions** (MuseScore's shortcuts v2, `MUSE_MODULE_SHORTCUTS_V2`): `src/App/configs/data/shortcuts.json` maps key sequences to `command://...` URIs. Orchestrion's handlers are still actions, so each shortcut-bound one has a `command://orchestrion/<name>` that dispatches it (`OrchestrionShell/internal/OrchestrionCommands.cpp`); add a shortcut by adding a command there and a line to the JSON. Keys bound twice are settled by their JSON scope (`OrchestrionShortcutsResolver`): `PLAYBACK` (Space, ←/→, ...) beats `NAVIGATION`, except while keyboard navigation has a control focused (from Tab until Esc, a mouse press or the controls fading out), when the arrows and Space go to the focused control. Every bound key becomes a Qt `Shortcut`, taken from whatever has the focus unless that item accepts the key's `ShortcutOverride` (as `ScoreShelf.qml`'s items do for Tab, Return, ...): that's why Return, Enter and Esc aren't bound (Esc leaves keyboard navigation through `KeyboardNavigationModel`'s event filter). MuseScore's own actions are being turned into commands upstream: dispatching a removed action code (e.g. `"loop"`) silently does nothing — dispatch the command (`ICommandDispatcher`) instead.
- **Doc comments**: for class and method documentation use the multiline Javadoc-style block
  ```cpp
  /**
   * comment
   */
  ```
  rather than `//! comment` (existing `//!` comments are legacy; don't introduce new ones).
