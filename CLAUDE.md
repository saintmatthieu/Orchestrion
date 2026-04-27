# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Orchestrion is a Qt 6 / C++17 desktop application built on top of a forked MuseScore. It reuses MuseScore's modular framework (the `muse::modularity` IoC container, the `IApplication` lifecycle, and most of the `mu::` / `muse::` modules — engraving, notation, project, playback, audio, vst, etc.) and adds Orchestrion-specific modules that turn the score into something the user can "play" via gesture controllers / external MIDI devices.

The MuseScore fork lives in the `MuseScore/` git submodule (https://github.com/saintmatthieu/MuseScore.git). Treat it as upstream code — most build glue (`MuseScore.cmake`) is adapted from upstream and points `MUSE_FRAMEWORK_PATH` and `MuseScore_ROOT_DIR` at the submodule. `DSPFilters/` is the second submodule. **Always clone with `--recurse-submodules`** (or run `git submodule update --init --recursive`); a missing submodule manifests as confusing missing-target errors during CMake configuration.

Git LFS is used for `*.jpg` (see `.gitattributes`) — run `git lfs pull` after a fresh clone.

## Build

CMake + Ninja, Qt 6.9.1, VST3 SDK (built from MuseScore's vendored sources). C++17, Allman braces (`.clang-format`).

```bash
# Configure (one-time; pick a build dir per config)
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF \
  -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=OFF \
  -DSMTG_ENABLE_VSTGUI_SUPPORT=OFF

# Build + install (install is required to lay out the runtime tree the launcher expects)
cmake --build build --target install
```

`CMAKE_INSTALL_PREFIX` is forced to `${CMAKE_BINARY_DIR}/install` on Linux/Windows and to the binary dir itself on macOS — do not override it. The `.vscode/launch.json` `preLaunchTask` is `CMake: install`, not just build, for this reason.

The Linux executable lives at `build/install/bin/Orchestrion`. On Linux the binary is placed in a `bin/` subdirectory of the build tree on purpose: the Qt QML module creates an `Orchestrion/` directory that would otherwise collide with the extension-less executable name (see comment in top-level `CMakeLists.txt`).

CI mirrors this with `ci_build.cmake` (configures + builds + installs under `build.release`), invoked from `ci_build.bat` on Windows. The GitHub workflow (`.github/workflows/orchestrion.yml`) builds Windows / macOS / Linux and packages NSIS / DMG / AppImage respectively.

There is **no test target** wired into the top-level build (`MUSE_BUILD_UNIT_TESTS` is forced OFF). Don't invent a `ctest` step — there's nothing to run.

## Runtime layout / data files

The install tree contains soundfonts, instrument definitions, scores, wallpapers, icons, and Qt's `qml/` and `translations/` trees. The locations differ per platform (`Mscore_SHARE_NAME` / `Mscore_INSTALL_NAME` in the root `CMakeLists.txt`) — if you touch resource installation, check all three branches.

Translations are produced from `share/locale/*.ts` via `qt_add_lrelease`; the install step also copies Qt's bundled `qt_*.qm` (excluding `qt_help_*` and `qtbase_*`).

On Windows, `libsndfile.dll` (`SNDFILE_DLL`) must be discoverable — the configure step fails with `FATAL_ERROR` if it isn't, since FluidSynth needs it to load soundfonts.

## Architecture

### Modular IoC, MuseScore-style

Each top-level directory in `src/` (except `App`, `OrchestrionCommon`, `qml`, `stubs`) is a **module** following MuseScore's framework conventions:

- `<Module>Module.{h,cpp}` — module entry point (registers services, types, QML imports).
- `I<Service>.h` at the module root — public interfaces consumed via the IoC container.
- `internal/` — implementations, registered with the container by the module's `registerExports`.
- `view/` — QML-backed C++ view-models (`QObject` subclasses).
- `qml/` — module-local QML imported via `MODULE_QML_IMPORT`.
- `<Module>.qrc` — Qt resources (icons, QML, etc.).

`CMakeLists.txt` files use the MuseScore framework macros: `declare_module(...)`, populate `MODULE_SRC` / `MODULE_LINK` / `MODULE_QML_IMPORT` / `MODULE_QRC`, then `setup_module()`. Most Orchestrion modules set `MODULE_NOT_LINK_GLOBAL ON`, `MODULE_USE_PCH_NONE ON`, `MODULE_USE_UNITY_NONE ON`. Follow the same pattern for new modules.

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
| `GestureControllers` | Touchpad / computer-keyboard / generic gesture-controller abstractions |
| `OrchestrionConfiguration` | App-wide configuration / preferences |

The factory lives in `src/App/OrchestrionAppFactory.cpp` — when adding a new module, register it there alongside the existing `app->addModule(...)` calls and add it to the top-level `CMakeLists.txt` (`add_subdirectory` + `target_link_libraries`).

### Run modes

`main.cpp` branches on `CommandLineParser::runMode()`:
- `GuiApp` (default) — full `QApplication` + UI.
- `AudioPluginRegistration` — headless `QCoreApplication` for VST scanning.

The console-app construction path is `OrchestrionAppFactory::newConsoleApp` (don't lump it together with the GUI path).

### QML / UI

Top-level QML is registered via `qt_add_qml_module(Orchestrion URI Orchestrion ...)` in the root `CMakeLists.txt` (currently `Main.qml`, `PlaybackButton.qml`). Module-internal QML lives under each module's `qml/` and is imported via `MODULE_QML_IMPORT` from that module's `CMakeLists.txt`. See `qml-architecture.md` for the high-level shell composition (`AppWindow → WindowContent → {HomePage, NotationPage, DockToolBar, ...}`).

## Conventions to respect

- **Namespace**: project-local code is in `namespace dgk` (see `OrchestrionAppFactory`, `CommandLineParser`, etc.). MuseScore code uses `mu::` / `muse::`.
- **Application name** is intentionally *not* `"Orchestrion"` for stable builds in some places — `main.cpp` sets it to `"OrchestrionDevelopment"` under `MUSE_APP_UNSTABLE`. The comment there explains why (changing it would lose user settings); preserve that.
- **Linux-only link-order workaround** at the bottom of the root `CMakeLists.txt`: `vst_sdk_3` must explicitly depend on `sdk_common`. Don't "clean up" that block.
- **GCC needs `-include cstring`** added globally (top of root `CMakeLists.txt`) because some MuseScore sources use `memset`/`memcpy` without including the header. Keep it.
- License headers: existing files use a GPL-3.0-or-later header (`Copyright (C) 2024 Matthieu Hodgkinson`). Match it on new files.
