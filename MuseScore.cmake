# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
#
# MuseScore
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore BVBA and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Modified by Matthieu Hodgkinson for the purpose of the project.
#
# This file is Orchestrion's adaptation of the MuseScore fork's root
# CMakeLists.txt: it configures and adds the Muse framework (the `muse`
# submodule of the MuseScore submodule), the MuseScore modules Orchestrion
# uses, and the third-party dependencies (via `muse_deps`), without building
# the MuseScore Studio application itself.

set(MuseScore_ROOT_DIR ${PROJECT_SOURCE_DIR}/MuseScore)

if (NOT EXISTS "${MuseScore_ROOT_DIR}/muse/framework/CMakeLists.txt" OR NOT EXISTS "${MuseScore_ROOT_DIR}/muse_deps/buildtools/manifest.cmake")
    message(FATAL_ERROR "Nested submodules of MuseScore are missing; run: git submodule update --init --recursive")
endif()

set(MUSE_FRAMEWORK_PATH "${MuseScore_ROOT_DIR}/muse")
set(MUSE_FRAMEWORK_SRC_PATH ${MUSE_FRAMEWORK_PATH}/framework)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH
    ${MuseScore_ROOT_DIR}
    ${MuseScore_ROOT_DIR}/buildscripts
    ${MuseScore_ROOT_DIR}/buildscripts/cmake
    ${MUSE_FRAMEWORK_SRC_PATH}/cmake
    ${MUSE_FRAMEWORK_PATH}/buildscripts/cmake
    ${CMAKE_MODULE_PATH}
    )

###########################################
# Setup option and build settings
###########################################
include(GetPaths)

set(MUSESCORE_BUILD_CONFIGURATION "app" CACHE STRING "Build configuration")
set(MUSE_APP_BUILD_MODE "dev" CACHE STRING "Build mode")
# Possible MUSE_APP_BUILD_MODE values:
# - dev     - for development/nightly builds
# - testing - for testing versions (alpha, beta, RC)
# - release - for stable release builds
set(MUSESCORE_REVISION "" CACHE STRING "Build revision")

# Framework modules Orchestrion does not need (a stub is used where the
# framework provides one). Set before MuseDeclareOptions so the option()
# calls there pick these up (CMP0077).
set(MUSE_ENABLE_UNIT_TESTS OFF)
set(MUSE_MODULE_AUDIO_EXPORT OFF)            # no mp3/ogg/flac/aac export: avoids the encoder dependencies
set(MUSE_MODULE_AUDIOPLUGINS OFF)            # Orchestrion provides its own stub (src/stubs/audioplugins)
set(MUSE_MODULE_AUTOMATION OFF)
set(MUSE_MODULE_CLOUD OFF)
set(MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT OFF)
set(MUSE_MODULE_LEARN OFF)
set(MUSE_MODULE_MEDIA ON) # the media stub does not compile (missing include path); the real module has no extra deps
set(MUSE_MODULE_MIDIREMOTE OFF)
set(MUSE_MODULE_MUSESAMPLER OFF)
set(MUSE_MODULE_TESTFLOW OFF)
set(MUSE_MODULE_TOURS OFF)
set(MUSE_MODULE_UPDATE OFF)
set(MUSE_MODULE_VST ON)
set(MUSE_COMPILE_USE_UNITY ON)
set(MUSE_COMPILE_USE_PCH ON)
option(MUSE_COMPILE_USE_COMPILER_CACHE "Try to use compiler cache" ON)

include(${MUSE_FRAMEWORK_SRC_PATH}/cmake/MuseDeclareOptions.cmake)

# MuseScore modules (mirrors MuseScore/CMakeLists.txt; OFF for what Orchestrion doesn't use).
# Plain variables rather than option(): this is a fixed configuration, and
# changes here must not be shadowed by stale cache entries.
set(MUE_BUILD_APPSHELL_MODULE ON)
set(MUE_BUILD_APPSHELL_QML ${MUE_BUILD_APPSHELL_MODULE})

set(MUE_BUILD_BRAILLE_MODULE OFF)
set(MUE_BUILD_BRAILLE_QML ON) # appshell_qml imports the module's QML (the stub provides it too)
set(MUE_BUILD_BRAILLE_TESTS OFF)

set(MUE_BUILD_CONVERTER_MODULE OFF)
set(MUE_BUILD_CONVERTER_TESTS OFF)

set(MUE_BUILD_ENGRAVING_QML ON)
set(MUE_BUILD_ENGRAVING_TESTS OFF)
# appshell_qml imports engraving_qml, which only exists with the devtools
set(MUE_BUILD_ENGRAVING_DEVTOOLS ON)
set(MUE_BUILD_ENGRAVING_PLAYBACK ON)

# IMPORT EXPORT MODULES
set(MUE_BUILD_IMPEXP_BB_MODULE OFF)
set(MUE_BUILD_IMPEXP_BWW_MODULE OFF)
set(MUE_BUILD_IMPEXP_CAPELLA_MODULE OFF)
set(MUE_BUILD_IMPEXP_MIDI_MODULE ON)
set(MUE_BUILD_IMPEXP_MLSCORE_MODULE OFF)
set(MUE_BUILD_IMPEXP_MNX_MODULE OFF)
set(MUE_BUILD_IMPEXP_MUSEDATA_MODULE OFF)
set(MUE_BUILD_IMPEXP_MUSICXML_MODULE ON)
set(MUE_BUILD_IMPEXP_OVE_MODULE OFF)
set(MUE_BUILD_IMPEXP_AUDIOEXPORT_MODULE OFF)
set(MUE_BUILD_IMPEXP_IMAGESEXPORT_MODULE OFF)
set(MUE_BUILD_IMPEXP_GUITARPRO_MODULE OFF)
set(MUE_BUILD_IMPEXP_MEI_MODULE OFF)
set(MUE_BUILD_IMPEXP_VIDEOEXPORT_MODULE OFF)
set(MUE_BUILD_IMPEXP_TABLEDIT_MODULE OFF)
set(MUE_BUILD_IMPEXP_LYRICS_MODULE OFF)
set(MUE_BUILD_IMPORTEXPORT_TESTS OFF)

set(MUE_BUILD_PROPERTIESPANEL_MODULE OFF)
set(MUE_BUILD_PROPERTIESPANEL_QML ON) # appshell_qml imports the module's QML (the stub provides it too)

# appshell_qml imports instrumentsscene_qml, and the stub has no QML
set(MUE_BUILD_INSTRUMENTSSCENE_MODULE ON)
set(MUE_BUILD_INSTRUMENTSSCENE_QML ${MUE_BUILD_INSTRUMENTSSCENE_MODULE})

set(MUE_BUILD_MUSESOUNDS_MODULE OFF)
set(MUE_BUILD_MUSESOUNDS_QML ON) # appshell_qml imports the module's QML (the stub provides it too)

set(MUE_BUILD_NOTATION_MODULE ON)
set(MUE_BUILD_NOTATION_TESTS OFF)

set(MUE_BUILD_NOTATIONSCENE_MODULE ON)
set(MUE_BUILD_NOTATIONSCENE_QML ${MUE_BUILD_NOTATIONSCENE_MODULE})
set(MUE_BUILD_NOTATIONSCENE_TESTS OFF)

set(MUE_BUILD_PALETTE_MODULE OFF)
set(MUE_BUILD_PALETTE_QML ON) # appshell_qml imports the module's QML (the stub provides it too)

set(MUE_BUILD_PLAYBACK_MODULE ON)
set(MUE_BUILD_PLAYBACK_QML ${MUE_BUILD_PLAYBACK_MODULE})
set(MUE_BUILD_PLAYBACK_TESTS OFF)

set(MUE_BUILD_PREFERENCES_MODULE ON) # appshell_qml imports preferences_qml
set(MUE_BUILD_PRINT_MODULE OFF)

set(MUE_BUILD_PROJECT_MODULE ON)
set(MUE_BUILD_PROJECT_QML ${MUE_BUILD_PROJECT_MODULE})
set(MUE_BUILD_PROJECT_TESTS OFF)

set(MUE_BUILD_MACOS_INTEGRATION OFF)

# === Setup ===
option(MUE_DOWNLOAD_SOUNDFONT "Download the latest soundfont version as part of the build process" ON)

# === Pack ===
set(MUE_RUN_LRELEASE OFF) # Orchestrion generates its own .qm files
option(MUE_INSTALL_SOUNDFONT "Install sound font" ON)

# === Tests ===
set(APP_WORKSPACE_CONFIG_FILE "${MuseScore_ROOT_DIR}/src/app/configs/workspaces.cfg")
set(APP_BUILTIN_WORKSPACES_DIR "${MuseScore_ROOT_DIR}/share/workspaces")

# === Debug ===
option(MUE_ENABLE_ENGRAVING_RENDER_DEBUG "Enable rendering debug" OFF)
option(MUE_ENABLE_ENGRAVING_LD_ACCESS "Enable diagnostic engraving check layout data access" OFF)
option(MUE_ENABLE_ENGRAVING_LD_PASSES "Enable engraving layout by passes" OFF)

###########################################
# Setup Configure
###########################################

set(QT_ADD_LINGUISTTOOLS ON)
set(QT_ADD_CONCURRENT ON)
set(QT_ADD_WEBSOCKET OFF)
set(QT_QPROCESS_SUPPORTED ON)
set(QT_CONCURRENT_SUPPORTED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)
set(MUSE_MODULE_MULTIWINDOWS_SINGLEPROC_MODE ON)
# The vendored KDDockWidgets v1 backend; v2 would be an extra source dependency.
set(MUSE_MODULE_DOCKWINDOW_KDDOCKWIDGETS_V2 OFF)

include(GetPlatformInfo)

# Where the app's resources land in the install tree (and where the app looks
# for them at run time). Must be set before SetupConfigure (see the fork's
# SetupConfigure.cmake), which bakes it into muse_framework_config.h.
if (OS_IS_MAC)
    set(MUSE_APP_INSTALL_RESOURCES_LOCATION "Orchestrion.app/Contents/Resources")
elseif (OS_IS_WIN)
    set(MUSE_APP_INSTALL_RESOURCES_LOCATION ".")
else()
    set(MUSE_APP_INSTALL_RESOURCES_LOCATION "share")
endif()

# SetupConfigure (from the MuseScore root) resolves paths relative to the
# MuseScore tree (version.cmake, src/app/app_config.h.in, ...).
set(PREV_PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(PROJECT_SOURCE_DIR ${MuseScore_ROOT_DIR})
include(${MuseScore_ROOT_DIR}/SetupConfigure.cmake)
set(PROJECT_SOURCE_DIR ${PREV_PROJECT_SOURCE_DIR})

###########################################
# Setup compiler and build environment
###########################################

include(SetupBuildEnvironment)

if (MUSE_COMPILE_USE_COMPILER_CACHE)
    include(SetupCompilerCache)
endif(MUSE_COMPILE_USE_COMPILER_CACHE)

###########################################
# Setup external dependencies
###########################################
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_BASE_DIR ${PROJECT_BINARY_DIR}/_deps)

include(SetupQt6)

if (MUE_DOWNLOAD_SOUNDFONT)
    set(PROJECT_SOURCE_DIR ${MuseScore_ROOT_DIR})
    include(DownloadSoundFont)
    set(PROJECT_SOURCE_DIR ${PREV_PROJECT_SOURCE_DIR})
endif(MUE_DOWNLOAD_SOUNDFONT)

# Third-party dependencies, resolved by muse_deps (prebuilt archives, or
# sources for header-only / in-tree deps such as the VST3 SDK).
set(EXTDEPS_DIR "${MuseScore_ROOT_DIR}/muse_deps")
set(LOCAL_ROOT_PATH ${FETCHCONTENT_BASE_DIR})
include(${EXTDEPS_DIR}/buildtools/manifest.cmake)
include(${MUSE_FRAMEWORK_PATH}/buildscripts/cmake/ExtDepsManifest.cmake)
extdeps_install_consumed(MACOS_BUNDLE Orchestrion.app)

# Populated by ExtDepsManifest (MUSE_MODULE_VST); Orchestrion's own code
# includes VST3 SDK headers directly.
get_property(vst3sdk_SOURCE_DIR GLOBAL PROPERTY vst3sdk_SOURCE_DIR)

###########################################
# Add source tree
###########################################

# The framework's and MuseScore's CMake files resolve include paths etc.
# relative to PROJECT_SOURCE_DIR, which they expect to be the MuseScore root.
set(PROJECT_SOURCE_DIR ${MuseScore_ROOT_DIR})

add_subdirectory(MuseScore/share/sound)

add_subdirectory(${MUSE_FRAMEWORK_SRC_PATH} MuseScore/muse/framework)

# MuseScore modules (mirrors MuseScore/src/CMakeLists.txt, minus the app)
if (MUE_BUILD_BRAILLE_MODULE)
    add_subdirectory(MuseScore/src/braille)
else()
    add_subdirectory(MuseScore/src/stubs/braille)
endif()

add_subdirectory(MuseScore/src/context)

if (MUE_BUILD_CONVERTER_MODULE)
    add_subdirectory(MuseScore/src/converter)
endif()

add_subdirectory(MuseScore/src/engraving)

add_subdirectory(MuseScore/src/importexport)

if (MUE_BUILD_PROPERTIESPANEL_MODULE)
    add_subdirectory(MuseScore/src/propertiespanel)
else()
    add_subdirectory(MuseScore/src/stubs/propertiespanel)
endif()

if (MUE_BUILD_INSTRUMENTSSCENE_MODULE)
    add_subdirectory(MuseScore/src/instrumentsscene)
else()
    add_subdirectory(MuseScore/src/stubs/instrumentsscene)
endif()

if (MUE_BUILD_MUSESOUNDS_MODULE)
    add_subdirectory(MuseScore/src/musesounds)
else()
    add_subdirectory(MuseScore/src/stubs/musesounds)
endif()

if (MUE_BUILD_NOTATION_MODULE)
    add_subdirectory(MuseScore/src/notation)
else()
    add_subdirectory(MuseScore/src/stubs/notation)
endif()

if (MUE_BUILD_NOTATIONSCENE_MODULE)
    add_subdirectory(MuseScore/src/notationscene)
else()
    add_subdirectory(MuseScore/src/stubs/notationscene)
endif()

if (MUE_BUILD_PALETTE_MODULE)
    add_subdirectory(MuseScore/src/palette)
else()
    add_subdirectory(MuseScore/src/stubs/palette)
endif()

if (MUE_BUILD_PLAYBACK_MODULE)
    add_subdirectory(MuseScore/src/playback)
else()
    add_subdirectory(MuseScore/src/stubs/playback)
endif()

if (MUE_BUILD_PREFERENCES_MODULE)
    add_subdirectory(MuseScore/src/preferences)
endif()

if (MUE_BUILD_PRINT_MODULE)
    add_subdirectory(MuseScore/src/print)
endif()

if (MUE_BUILD_PROJECT_MODULE)
    add_subdirectory(MuseScore/src/project)
endif()

if (MUE_BUILD_APPSHELL_MODULE)
    add_subdirectory(MuseScore/src/appshell)
endif()

set(PROJECT_SOURCE_DIR ${PREV_PROJECT_SOURCE_DIR})
set(PREV_PROJECT_SOURCE_DIR "")

# Legacy declare_module()/setup_module() macros used by Orchestrion's modules
include(DeclareModuleSetup)

# Orchestrion's targets (declared after this point) use Qt directly
link_libraries(${QT_LIBRARIES})

# Include paths for Orchestrion's own code: MuseScore's modules and the framework
include_directories(
    ${PROJECT_BINARY_DIR}
    ${MuseScore_ROOT_DIR}/src
    ${MUSE_FRAMEWORK_PATH}
    ${MUSE_FRAMEWORK_SRC_PATH}
    ${MUSE_FRAMEWORK_SRC_PATH}/global
)
