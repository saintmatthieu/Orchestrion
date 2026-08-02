# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2024 Matthieu Hodgkinson
#
# harfbuzz.cmake — Populate function for harfbuzz 7.1.0
#
# This file is a workaround for the musescore/muse_deps repository
# restructuring (June 2026) that removed the old
# harfbuzz/7.1.0/harfbuzz.cmake helper scripts. It defines the
# harfbuzz_Populate() CMake function that is expected by
# MuseScore/src/framework/draw/cmake/SetupHarfBuzz.cmake.
#
# Usage: copy (or symlink) this file to
#   MuseScore/src/framework/draw/_deps/harfbuzz/harfbuzz.cmake
# before running cmake configure. The CI workflow does this automatically
# (see .github/workflows/orchestrion.yml).

function(harfbuzz_Populate remote_url local_path type arg1 arg2)
    set(_hb_wrapper_dir "${local_path}/harfbuzz")
    if (EXISTS "${_hb_wrapper_dir}/CMakeLists.txt")
        return()
    endif()

    set(_hb_archive "${local_path}/harfbuzz-7.1.0.tar.xz")
    set(_hb_url "https://github.com/harfbuzz/harfbuzz/releases/download/7.1.0/harfbuzz-7.1.0.tar.xz")

    if (NOT EXISTS "${_hb_archive}")
        message(STATUS "Downloading harfbuzz 7.1.0 from ${_hb_url}...")
        file(DOWNLOAD "${_hb_url}" "${_hb_archive}"
            SHOW_PROGRESS
            STATUS _download_status
        )
        list(GET _download_status 0 _download_code)
        if (NOT _download_code EQUAL 0)
            message(FATAL_ERROR "Failed to download harfbuzz 7.1.0: ${_download_status}")
        endif()
    endif()

    message(STATUS "Extracting harfbuzz 7.1.0...")
    file(ARCHIVE_EXTRACT
        INPUT "${_hb_archive}"
        DESTINATION "${local_path}"
    )

    # Reproduce the original muse_deps layout: a small wrapper CMake project
    # at <local_path>/harfbuzz that builds the amalgam
    # (harfbuzz/src/harfbuzz.cc) as a muse module, with the actual harfbuzz
    # release nested one level deeper. SetupHarfBuzz.cmake's add_subdirectory
    # and HARFBUZZ_INCLUDE_DIRS both assume this shape. Extracting the
    # release directly in the wrapper's place (as this script used to do)
    # made add_subdirectory run harfbuzz's own build system instead, whose
    # find_package(Freetype REQUIRED) fails on the CI runners.
    file(MAKE_DIRECTORY "${_hb_wrapper_dir}")
    file(RENAME "${local_path}/harfbuzz-7.1.0" "${_hb_wrapper_dir}/harfbuzz")

    # Byte-for-byte copy of the wrapper CMakeLists.txt that the original
    # muse_deps populate script used to install.
    file(WRITE "${_hb_wrapper_dir}/CMakeLists.txt" [=[
# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
#
# MuseScore
# Music Composition & Notation
#
# Copyright (C) 2024 MuseScore BVBA and others
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

declare_module(harfbuzz)
set(MODULE_DIR ${CMAKE_CURRENT_LIST_DIR}/harfbuzz)

set(MODULE_SRC
    ${MODULE_DIR}/src/harfbuzz.cc
)

set(MODULE_INCLUDE
    ${FREETYPE_INCLUDE_DIRS}
)

set(MODULE_DEF
    -DHAVE_FREETYPE
)

set(MODULE_NOT_LINK_GLOBAL ON)
set(MODULE_PCH_DISABLED ON)
set(MODULE_UNITY_DISABLED ON)
setup_module()

# target_no_warning(${MODULE} -Wimplicit-fallthrough=0)
# target_no_warning(${MODULE} -Wno-conversion)
# target_no_warning(${MODULE} -Wno-cast-function-type)
]=])
endfunction()
