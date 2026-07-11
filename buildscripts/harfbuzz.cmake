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
    set(_hb_source_dir "${local_path}/harfbuzz")
    if (EXISTS "${_hb_source_dir}/CMakeLists.txt")
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
    file(RENAME "${local_path}/harfbuzz-7.1.0" "${_hb_source_dir}")
endfunction()
