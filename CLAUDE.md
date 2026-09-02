# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Orchestrion is a Qt 6 / C++20 desktop application built on top of a forked MuseScore. It reuses MuseScore's modular framework (the `muse::modularity` IoC container, the `IApplication` lifecycle, and most of the `mu::` / `muse::` modules — engraving, notation, project, playback, audio, vst, etc.) and adds Orchestrion-specific modules that turn the score into something the user can "play" via gesture controllers / external MIDI devices.

The MuseScore fork lives in the `MuseScore/` git submodule (https://github.com/saintmatthieu/MuseScore.git, branch `orchestrion-<date>`, a handful of Orchestrion commits on top of upstream `main` — MuseScore 5.0-dev). Treat it as upstream code — the build glue (`MuseScore.cmake`) is adapted from upstream's root `CMakeLists.txt`. MuseScore itself has two nested submodules: `MuseScore/muse` (the Muse framework, https://github.com/musescore/muse_framework — `MUSE_FRAMEWORK_PATH`) and `MuseScore/muse_deps` (recipes and a lock file for prebuilt third-party libraries, downloaded at configure time). `DSPFilters/` is the other top-level submodule. **Always clone with `--recurse-submodules`** (or run `git submodule update --init --recursive`); a missing submodule manifests as a `FATAL_ERROR` about nested submodules or confusing missing-target errors during CMake configuration.

Git LFS is used for `*.jpg` (see `.gitattributes`) — run `git lfs pull` after a fresh clone.

## Build

CMake 3.28+ (3.31+ on macOS) + Ninja, Qt 6.8+ (CI: 6.9.1; local Macs: 6.10.x), C++20 (upstream wants AppleClang 21 / GCC 14 / MSVC 19.40; AppleClang 17 works, the version check only warns). Allman braces (`.clang-format`) for Orchestrion code.

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

There is **no test target** wired into the top-level build (`MUSE_BUILD_UNIT_TESTS` is forced OFF). Don't invent a `ctest` step — there's nothing to run.

## Runtime layout / data files

The install tree contains soundfonts, scores, wallpapers, icons, the prebuilt dependency libraries, and Qt's `translations/` tree. The locations differ per platform (`MUSE_APP_INSTALL_RESOURCES_LOCATION` in `MuseScore.cmake`, which MuseScore's `share/` rules use, and `resource_install_dir` in the root `CMakeLists.txt`) — if you touch resource installation, check all three branches.

Translations are produced from `share/locale/*.ts` via `qt_add_lrelease`; the install step also copies Qt's bundled `qt_*.qm` (excluding `qt_help_*` and `qtbase_*`).

## Architecture

### Modular IoC, MuseScore-style

MuseScore's framework lives in `MuseScore/muse/framework/<module>` (`global`, `ui`, `uicomponents`, `audio`, `vst`, `midi`, `interactive`, `multiwindows`, ...; include as `<module>/header.h` or `framework/<module>/header.h`) and MuseScore's own modules in `MuseScore/src/<module>` (`engraving`, `notation`, `notationscene`, `playback`, `project`, `appshell`, ...). QML-backed view classes now live inside the modules' QML directories, e.g. `notationscene/qml/MuseScore/NotationScene/notationpaintview.h` and `appshell/qml/MuseScore/AppShell/mainwindowtitleprovider.h`. `MuseScore.cmake` lists which MuseScore/framework modules are built (`MUE_BUILD_*` / `MUSE_MODULE_*` as plain `set()`s — they are a fixed configuration; stubs are used for the rest).

The framework has a **global IoC** (`globalIoc()`, `GlobalInject<I>`, interfaces declared `MODULE_GLOBAL_INTERFACE`) and **per-window context IoCs** (`ioc(ctx)`, `ContextInject<I>`, `MODULE_CONTEXT_INTERFACE`). Orchestrion is a single-window app: `src/OrchestrionCommon/OrchestrionIoc.h` records the one context (`dgk::iocContext()`), and Orchestrion classes derive from `dgk::Injectable` and declare services as `dgk::Inject<I> name{this};` — it picks `GlobalInject` or `ContextInject` from the interface's declaration (a context miss falls back to the global IoC). Orchestrion's own interfaces are `MODULE_GLOBAL_EXPORT_INTERFACE` and are registered in `globalIoc()` from `registerExports()`. Anything that touches MuseScore's context-scoped services (`IGlobalContext`, `IPlaybackController`, `IPlayback`, `IUiActionsRegister`, ...) runs in the **context phase**: each Orchestrion module implements `newContext(ctx)` returning a `dgk::ModuleContextSetup` whose hooks (`onInit`, `onAllInited`, `resolveImports`) forward to the module's `onContext*` methods.

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

MuseScore's audio engine runs on its own thread and talks to the main thread over an RPC channel (`audio/main` = app side, `audio/engine` = engine side). `OrchestrionSynthesis` registers `OrchestrionSynthResolver` for the built-in (`Fluid`) source type once the audio has started and routes the score's tracks to it with `IPlayback::setSourceParams`; the wanted synthesizer (built-in FluidSynth, a VST, or none) is carried in the params' `resourceMeta.attributes`, so changing it changes the params and makes the engine re-resolve the track's synthesizer (identical params are a no-op). `OrchestrionSynthesizerWrapper` implements `ISynthesizer` and is fed by the gesture sequencer, not by the score's playback data; the engine processes it while idle too.

### Run modes

`main.cpp` branches on `CommandLineParser::runMode()`:
- `GuiApp` (default) — full `QApplication` + UI.
- `AudioPluginRegistration` — headless `QCoreApplication` for VST scanning.

The console-app construction path is `OrchestrionAppFactory::newConsoleApp` (don't lump it together with the GUI path).

### QML / UI

Top-level QML is registered via `qt_add_qml_module(Orchestrion URI Orchestrion ...)` in the root `CMakeLists.txt`; every file in `src/qml/` must be listed there (`QML_FILES`) as well as in `resources.qrc`. Module-internal QML lives under each module's `qml/` and is imported via `MODULE_QML_IMPORT` from that module's `CMakeLists.txt`. See `qml-architecture.md` for the high-level shell composition (`AppWindow → WindowContent → {HomePage, NotationPage, DockToolBar, ...}`).

MuseScore's QML is made of compiled QML modules (`Muse.Ui`, `Muse.UiComponents`, `Muse.Interactive`, `MuseScore.AppShell`, `MuseScore.NotationScene`, ...): import them **without a version** (a versioned import of a module that ships scripts, such as `Muse.UiComponents`, is "ambiguous" against the unversioned dependency import another module pulls in), and in files that also use `QtQuick.Controls` import `Muse.UiComponents` (and modules depending on it) **qualified** — its `MenuItem` type would otherwise shadow the Controls one. Types can't be injected into those modules with `qmlRegisterType`; Orchestrion registers its own types under `Orchestrion.*` URIs (`OrchestrionMenuModel`, `MainWindowBridge` in `Orchestrion.MuseScoreShell`). The Windows/Linux window chrome (`OrchestrionTitleBar`, `OrchestrionMenuBar`, `OrchestrionSystemButtons`) and the macOS menu bar (`MacMenuBar`) are Orchestrion copies of MuseScore's, using `OrchestrionMenuModel`.

## Conventions to respect

- **Namespace**: project-local code is in `namespace dgk` (see `OrchestrionAppFactory`, `CommandLineParser`, etc.). MuseScore code uses `mu::` / `muse::`.
- **Application name** is intentionally *not* `"Orchestrion"` for stable builds in some places — `main.cpp` sets it to `"OrchestrionDevelopment"` under `MUSE_APP_UNSTABLE`. The comment there explains why (changing it would lose user settings); preserve that.
- **GCC needs `-include cstring`** added globally (top of root `CMakeLists.txt`) because some MuseScore sources use `memset`/`memcpy` without including the header. Keep it.
- **Fork patches to MuseScore** are a short series of commits on the submodule's `orchestrion-<date>` branch (engraving: `unrollRepeatsInPlace`, layout tick warp, `UndoStack::markClean`; notation: `OrchestrionNotationInteraction`, painting; notationscene: paint hooks; `version.cmake`; `SetupConfigure.cmake`). When bumping MuseScore again, rebase or re-apply those commits — nothing in the `muse` framework submodule is patched (it points at upstream's commit).
- **kors_async channels copy-assign their payload**: event value types sent through `muse::async::Channel` must be assignable (no `const` members).
- License headers: existing files use a GPL-3.0-or-later header (`Copyright (C) 2024 Matthieu Hodgkinson`). Match it on new files.
- **Doc comments**: for class and method documentation use the multiline Javadoc-style block
  ```cpp
  /**
   * comment
   */
  ```
  rather than `//! comment` (existing `//!` comments are legacy; don't introduce new ones).
