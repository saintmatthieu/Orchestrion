include(GetPlatformInfo)

if(NOT OS_IS_WIN)
    return()
endif()

#! NOTE: the following is true for cmake version 3.30.5. When upgrading cmake,
#! consider removing this if block.
# CMake's built-in detection of MSVC_REDIST_DIR only knows about Visual Studio
# versions 15-17 (2017-2022). VS 18 (2026) installs are not found, so the CRT
# DLLs end up missing from the MSI. Probe via vswhere and set MSVC_REDIST_DIR
# ourselves before InstallRequiredSystemLibraries consumes it.
if(NOT MSVC_REDIST_DIR OR NOT EXISTS "${MSVC_REDIST_DIR}")
    set(_pf_x86_env "ProgramFiles(x86)")
    set(_vswhere "$ENV{${_pf_x86_env}}/Microsoft Visual Studio/Installer/vswhere.exe")
    if(EXISTS "${_vswhere}")
        execute_process(
            COMMAND "${_vswhere}" -latest -property installationPath
            OUTPUT_VARIABLE _vs_install_path
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(_vs_install_path AND IS_DIRECTORY "${_vs_install_path}/VC/Redist/MSVC")
            file(GLOB _vs_redist_candidates "${_vs_install_path}/VC/Redist/MSVC/*")
            list(SORT _vs_redist_candidates ORDER DESCENDING)
            foreach(_candidate IN LISTS _vs_redist_candidates)
                if(IS_DIRECTORY "${_candidate}/x64/Microsoft.VC143.CRT")
                    set(MSVC_REDIST_DIR "${_candidate}" CACHE PATH "MSVC redistributable directory" FORCE)
                    message(STATUS "Detected MSVC_REDIST_DIR via vswhere: ${MSVC_REDIST_DIR}")
                    break()
                endif()
            endforeach()
            unset(_vs_redist_candidates)
        endif()
        unset(_vs_install_path)
    endif()
    unset(_vswhere)
    unset(_pf_x86_env)
endif()

include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME ${MUSE_APP_NAME})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Orchestrion, a piano practice and performance application") 
set(CPACK_PACKAGE_VENDOR "saintmatthieu")
# set(CPACK_PACKAGE_CONTACT "https://musescore.org")
# set(CPACK_PACKAGE_HOMEPAGE_URL "https://musescore.org")

set(CPACK_PACKAGE_VERSION_MAJOR "${MUSE_APP_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MUSE_APP_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MUSE_APP_VERSION_PATCH}")
# WiX requires Product/@Version to be x.x.x.x with all four parts; CI sets
# CMAKE_BUILD_NUMBER, but local builds leave it empty — default to 0 so the
# .msi can still be produced.
if(NOT CMAKE_BUILD_NUMBER)
    set(CMAKE_BUILD_NUMBER 0)
endif()
set(CPACK_PACKAGE_VERSION_BUILD "${CMAKE_BUILD_NUMBER}")
set(CPACK_PACKAGE_VERSION "${MUSE_APP_VERSION_MAJOR}.${MUSE_APP_VERSION_MINOR}.${MUSE_APP_VERSION_PATCH}.${CPACK_PACKAGE_VERSION_BUILD}")
message("CPACK_PACKAGE_VERSION: ${CPACK_PACKAGE_VERSION}")

set(git_date_string "")

if(MUSE_APP_UNSTABLE)
    find_program(GIT_EXECUTABLE git PATHS ENV PATH)

    if(GIT_EXECUTABLE)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" log -1 --date=short --format=%cd
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE git_date
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()

    if(git_date)
        string(REGEX REPLACE "-" "" git_date "${git_date}")
        set(git_date_string "~git${git_date}")
    endif()
endif(MUSE_APP_UNSTABLE)

set(CPACK_PACKAGE_FILE_NAME "${MUSE_APP_NAME}-${MUSE_APP_VERSION}${git_date_string}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${MUSE_APP_NAME_VERSION})

set(MUSESCORE_EXECUTABLE_NAME ${MUSE_APP_NAME}${MUSE_APP_VERSION_MAJOR})
set(CPACK_PACKAGE_EXECUTABLES "${MUSESCORE_EXECUTABLE_NAME}" "${MUSE_APP_TITLE_VERSION}") # exe name, label
set(CPACK_CREATE_DESKTOP_LINKS "${MUSESCORE_EXECUTABLE_NAME}" "${MUSE_APP_TITLE_VERSION}") # exe name, label

# Wix-specific options
set(CPACK_GENERATOR "WIX")

file(TO_CMAKE_PATH $ENV{PROGRAMFILES} PROGRAMFILES)
set(CPACK_WIX_ROOT "${PROGRAMFILES}/WiX Toolset v3.11")

# Use custom version of WIX.template.in
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/buildscripts/packaging/Windows/Installer" ${CMAKE_MODULE_PATH})

if(NOT CPACK_WIX_PRODUCT_GUID)
    set(CPACK_WIX_PRODUCT_GUID "00000000-0000-0000-0000-000000000000")
endif()

message(STATUS "[SetupWindowsPackaging.cmake] CPACK_WIX_PRODUCT_GUID: ${CPACK_WIX_PRODUCT_GUID}")

if(NOT CPACK_WIX_UPGRADE_GUID)
    set(CPACK_WIX_UPGRADE_GUID "11111111-1111-1111-1111-111111111111")
endif()

message(STATUS "[SetupWindowsPackaging.cmake] CPACK_WIX_UPGRADE_GUID: ${CPACK_WIX_UPGRADE_GUID}")

set(CPACK_WIX_LICENSE_RTF "${PROJECT_SOURCE_DIR}/buildscripts/packaging/Windows/Installer/LICENSE.rtf")
set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/icons/music-box.ico")

# Installer UI language. Defaults to en-US for the existing single-MSI flow.
# When CI builds per-language MSIs it overrides CPACK_WIX_CULTURES (and
# CPACK_WIX_LICENSE_RTF) via `cpack -D`. The matching strings_<culture>.wxl
# supplies localized strings referenced from WIX.template.in as !(loc.Id).
if(NOT CPACK_WIX_CULTURES)
    set(CPACK_WIX_CULTURES "en-US")
endif()
list(APPEND CPACK_WIX_LIGHT_EXTRA_FLAGS
    "-loc" "${PROJECT_SOURCE_DIR}/buildscripts/packaging/Windows/Installer/strings_${CPACK_WIX_CULTURES}.wxl"
)
set(CPACK_WIX_UI_BANNER "${PROJECT_SOURCE_DIR}/buildscripts/packaging/Windows/Installer/installer_banner_wix.png")
set(CPACK_WIX_UI_DIALOG "${PROJECT_SOURCE_DIR}/buildscripts/packaging/Windows/Installer/installer_background_wix.png")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "${MUSE_APP_TITLE_VERSION}")
set(CPACK_WIX_EXTENSIONS "WixUtilExtension")

# Extra CPack variables
list(APPEND CPACK_WIX_CANDLE_EXTRA_FLAGS
    "-dMUSE_APP_TITLE_VERSION=${MUSE_APP_TITLE_VERSION}"
    "-dMUSE_APP_TITLE=${MUSE_APP_TITLE}"
    "-dMUSESCORE_EXECUTABLE_NAME=${MUSESCORE_EXECUTABLE_NAME}"
    "-dMUSE_APP_RELEASE_CHANNEL=${MUSE_APP_RELEASE_CHANNEL}"
    "-dCPACK_PACKAGE_VERSION_MAJOR=${CPACK_PACKAGE_VERSION_MAJOR}"
)

if (MUSE_APP_IS_PRERELEASE)
    list(APPEND CPACK_WIX_CANDLE_EXTRA_FLAGS "-dMUSE_APP_IS_PRERELEASE=ON")
endif()

include(CPack)
