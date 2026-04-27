if (NOT WIN32)
    return()
endif()

set(CPACK_PACKAGE_NAME "Orchestrion")
set(CPACK_PACKAGE_VENDOR "saintmatthieu")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Play your scores with gesture controllers and external MIDI devices")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/saintmatthieu/Orchestrion")
set(CPACK_PACKAGE_CONTACT "saintmatthieu@mailbox.org")

set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

set(CPACK_PACKAGE_FILE_NAME "Orchestrion-${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Orchestrion")

# Start menu shortcut: <exe-basename> <label>
set(CPACK_PACKAGE_EXECUTABLES "Orchestrion" "Orchestrion")
set(CPACK_CREATE_DESKTOP_LINKS "Orchestrion")

set(CPACK_GENERATOR "WIX")

# Stable across releases. UpgradeCode in particular MUST NOT change once you ship,
# or upgrades will install side-by-side instead of replacing the previous version.
# Replace the placeholder once with a freshly generated GUID and commit it.
# (PowerShell: [guid]::NewGuid() | clip)
if (NOT CPACK_WIX_UPGRADE_GUID)
    set(CPACK_WIX_UPGRADE_GUID "REPLACE-ME-WITH-A-FRESH-GUID-FOR-UPGRADE")
endif()
# "*" tells WiX to mint a new ProductCode per build, which is what major upgrades want.
if (NOT CPACK_WIX_PRODUCT_GUID)
    set(CPACK_WIX_PRODUCT_GUID "*")
endif()

set(_orchestrion_installer_dir "${CMAKE_CURRENT_LIST_DIR}/Installer")

set(CPACK_WIX_LICENSE_RTF "${_orchestrion_installer_dir}/LICENSE.rtf")
set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/icons/music-box.ico")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "Orchestrion")
set(CPACK_WIX_UI_REF "WixUI_InstallDir")

# Optional: drop a 493x58 banner and a 493x312 background PNG next to LICENSE.rtf
# to brand the wizard. Without them WiX uses the default Microsoft graphics.
if (EXISTS "${_orchestrion_installer_dir}/installer_banner_wix.png")
    set(CPACK_WIX_UI_BANNER "${_orchestrion_installer_dir}/installer_banner_wix.png")
endif()
if (EXISTS "${_orchestrion_installer_dir}/installer_background_wix.png")
    set(CPACK_WIX_UI_DIALOG "${_orchestrion_installer_dir}/installer_background_wix.png")
endif()

# Custom template only adds the "Launch Orchestrion" checkbox to the finish page;
# everything else is plain WixUI_InstallDir.
set(CPACK_WIX_TEMPLATE "${_orchestrion_installer_dir}/WIX.template.in")
set(CPACK_WIX_EXTENSIONS "WixUtilExtension")

list(APPEND CPACK_WIX_CANDLE_EXTRA_FLAGS
    "-dORCHESTRION_EXECUTABLE_NAME=Orchestrion"
)

include(CPack)
