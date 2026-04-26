cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0000 NEW)
cmake_policy(SET CMP0079 NEW)

# Set the project name and version
project(Orchestrion VERSION 1.0 LANGUAGES C CXX)

# Find the Qt6 packages
find_package(Qt6 6.9.1 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 QuickLayouts Widgets ShaderTools)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# On Linux with GCC, some MuseScore source files use memset/memcpy without including <cstring>
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-include$<SEMICOLON>cstring>)
endif()

set(MuseScore_ROOT_DIR ${PROJECT_SOURCE_DIR}/MuseScore)
set(MUSE_FRAMEWORK_PATH ${PROJECT_SOURCE_DIR}/MuseScore)
set(MUSE_BUILD_UNIT_TESTS OFF)

add_compile_definitions(LIBSNDFILE_SUPPORT)

include_directories(${PROJECT_SOURCE_DIR}/src)

include(vst3sdk.cmake)
include(MuseScore.cmake)

# This statement must be after include(MuseScore.cmake), as it sets QT_INSTALL_LIBS
if (OS_IS_MAC)
    set(CMAKE_INSTALL_RPATH "${QT_INSTALL_LIBS};@loader_path/../Frameworks")
elseif (OS_IS_LIN)
    set(CMAKE_INSTALL_RPATH "${QT_INSTALL_LIBS};$ORIGIN/../lib")
else()
    set(CMAKE_INSTALL_RPATH "${QT_INSTALL_LIBS}")
endif()
if (OS_IS_MAC)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Installation prefix for macOS" FORCE)
else()
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation prefix for other OS" FORCE)
endif()

add_subdirectory(src/ExternalDevices)
add_subdirectory(src/GestureControllers)
add_subdirectory(src/MuseScoreShell)
add_subdirectory(src/OrchestrionConfiguration)
add_subdirectory(src/OrchestrionNotation)
add_subdirectory(src/OrchestrionOnboarding)
add_subdirectory(src/OrchestrionSequencer)
add_subdirectory(src/OrchestrionShell)
add_subdirectory(src/OrchestrionSynthesis)
add_subdirectory(src/ScoreAnimation)
add_subdirectory(src/stubs/audioplugins)

set(MACOSX_BUNDLE_ICON_FILE icons/music-box.icns)

qt_add_executable(Orchestrion WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/App/CommandLineParser.cpp
    src/App/CommandLineParser.h
    src/App/CommandOptions.h
    src/App/OrchestrionApp.cpp
    src/App/OrchestrionApp.h
    src/App/OrchestrionAppFactory.cpp
    src/App/OrchestrionAppFactory.h
)

set_target_properties(Orchestrion PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# On Linux, the QML module creates an "Orchestrion/" directory in the build tree,
# which conflicts with the executable name "Orchestrion" (no extension on Linux).
# Place the executable in a bin/ subdirectory to avoid the collision.
if (OS_IS_LIN)
    set_target_properties(Orchestrion PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

target_link_libraries(Orchestrion PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickLayouts
    Qt6::Widgets

    vst3sdk::sdk
    vst3sdk::sdk_hosting

    muse_actions
    muse_accessibility
    muse_audio
    muse_audioplugins
    commonscene
    context
    muse_draw
    engraving
    muse_extensions
    muse_global
    iex_midi
    iex_musicxml
    muse_languages
    muse_midi
    muse_mpe
    muse_multiinstances
    muse_network
    playback
    project
    muse_shortcuts
    muse_ui
    muse_uicomponents
    muse_update
    muse_vst
    muse_workspace

    ExternalDevices
    GestureControllers
    MusescoreShell
    OrchestrionConfiguration
    OrchestrionNotation
    OrchestrionOnboarding
    OrchestrionSequencer
    OrchestrionShell
    OrchestrionSynthesis
    ScoreAnimation
)

if (OS_IS_WIN)
    target_link_libraries(Orchestrion PRIVATE runtimeobject)
endif()

# On Linux, static library link order matters. The VST3 SDK's vst_sdk_3 (built by MuseScore)
# depends on symbols in sdk_common (built by the fetched VST3 SDK), but CMake places
# sdk_common before vst_sdk_3. Declaring the dependency ensures correct link ordering.
if (OS_IS_LIN)
    target_link_libraries(vst_sdk_3 PRIVATE sdk_common)
endif()

# Add QML files to the project (for IDEs and resource system)
qt_add_qml_module(Orchestrion
    URI Orchestrion
    VERSION 1.0
    QML_FILES "src/qml/Main.qml"
    QML_FILES "src/qml/PlaybackButton.qml"
)

qt_add_resources(RESOURCES resources.qrc)
qt_add_resources(RESOURCES src/App/OrchestrionApp.qrc)
qt_add_resources(RESOURCES src/OrchestrionSequencer/OrchestrionSequencer.qrc)

target_sources(Orchestrion PRIVATE
    ${RESOURCES}
)

# Install soundfonts
if (OS_IS_MAC)
    # Override this
    # SET(Mscore_SHARE_NAME      "mscore.app/")
    SET(Mscore_SHARE_NAME "Orchestrion.app/")
    SET(Mscore_INSTALL_NAME "Contents/Resources/")
elseif (OS_IS_LIN)
    SET(Mscore_SHARE_NAME "share/")
    SET(Mscore_INSTALL_NAME "")
elseif (OS_IS_WIN)
    SET(Mscore_SHARE_NAME "./")
    SET(Mscore_INSTALL_NAME "")
endif()
add_subdirectory(MuseScore/share/sound)
add_subdirectory(MuseScore/share/instruments)

add_subdirectory(DSPFilters/shared/DSPFilters)

if (OS_IS_MAC)
    set(resource_install_dir "Orchestrion.app/Contents/Resources")
elseif (OS_IS_LIN)
    set(resource_install_dir "${CMAKE_INSTALL_PREFIX}/share")
elseif (OS_IS_WIN)
    set(resource_install_dir ${CMAKE_INSTALL_PREFIX})
endif()
install(DIRECTORY "${CMAKE_SOURCE_DIR}/scores/"             DESTINATION ${resource_install_dir}/scores)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/wallpapers/"         DESTINATION ${resource_install_dir}/wallpapers)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/icons"               DESTINATION ${resource_install_dir})
install(DIRECTORY ${QT_INSTALL_QML}                         DESTINATION ${resource_install_dir})

install(TARGETS Orchestrion
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .)

# Ensure all required Qt runtime DLLs/plugins are deployed into the install tree.
# This prevents runtime failures on CI artifacts when new Qt runtime modules are introduced.
if (OS_IS_WIN)
    qt_generate_deploy_qml_app_script(
        TARGET Orchestrion
        OUTPUT_SCRIPT orchestrion_qt_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${orchestrion_qt_deploy_script})
endif()

### TRANSLATIONS ### Copied and adapted from MuseScore/share/locale/CMakeLists.txt
file(GLOB TS_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/share/locale/*.ts)

add_custom_target(translations ALL COMMENT "Generating .qm files" VERBATIM)
qt_add_lrelease(translations TS_FILES ${TS_FILES} NO_GLOBAL_TARGET QM_FILES_OUTPUT_VARIABLE QM_FILES)

install(
    FILES ${CMAKE_SOURCE_DIR}/share/locale/languages.json ${QM_FILES}
    DESTINATION ${resource_install_dir}/locale
)

install(DIRECTORY ${QT_INSTALL_PREFIX}/translations/
    DESTINATION ${Mscore_SHARE_NAME}${Mscore_INSTALL_NAME}locale
    FILES_MATCHING
    REGEX "qt_.*\\.qm"
    REGEX "qt_help_.*\\.qm" EXCLUDE
    REGEX "qtbase_.*\\.qm"
    REGEX "qt*_en\\.qm" EXCLUDE)
### TRANSLATIONS ###

if (OS_IS_WIN)
    if (SNDFILE_DLL)
        install(FILES ${SNDFILE_DLL} DESTINATION bin)
        # add the LIBSNDFILE_SUPPORT definition
        add_custom_command(TARGET Orchestrion POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SNDFILE_DLL} $<TARGET_FILE_DIR:Orchestrion>)
    else()
        message(FATAL_ERROR "libsndfile is needed by fluidsynth to load soundfonts")
    endif()
endif()
