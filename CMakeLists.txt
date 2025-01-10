cmake_minimum_required(VERSION 3.16)

# Set the project name and version
project(Orchestrion VERSION 1.0 LANGUAGES C CXX)

# Find the Qt6 packages
find_package(Qt6 6.2.4 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MuseScore_ROOT_DIR ${PROJECT_SOURCE_DIR}/MuseScore)
set(MUSE_FRAMEWORK_PATH ${PROJECT_SOURCE_DIR}/MuseScore)
set(MUSE_BUILD_UNIT_TESTS OFF)

include_directories(${PROJECT_SOURCE_DIR}/src)

include(vst3sdk.cmake)
include(MuseScore.cmake)
include(DSPFilters.cmake)

add_subdirectory(src/MusescoreShell)
add_subdirectory(src/Orchestrion)
add_subdirectory(src/OrchestrionNotation)
add_subdirectory(src/OrchestrionOnboarding)
add_subdirectory(src/OrchestrionShell)
add_subdirectory(src/OrchestrionSynthesis)
add_subdirectory(src/ScoreAnimation)

# Add the executable
qt_add_executable(OrchestrionApp WIN32
    src/main.cpp
    src/App/CommandLineParser.cpp
    src/App/CommandLineParser.h
    src/App/CommandOptions.h
    src/App/OrchestrionApp.cpp
    src/App/OrchestrionApp.h
    src/App/OrchestrionAppFactory.cpp
    src/App/OrchestrionAppFactory.h
)

set_target_properties(OrchestrionApp PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(OrchestrionApp PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets

    vst3sdk::sdk
    vst3sdk::sdk_hosting

    muse_actions
    muse_accessibility
    muse_audio
    muse_audioplugins
    braille
    commonscene
    context
    muse_diagnostics
    muse_draw
    engraving
    muse_extensions
    muse_global
    iex_guitarpro
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

    MusescoreShell
    Orchestrion
    OrchestrionNotation
    OrchestrionOnboarding
    OrchestrionShell
    OrchestrionSynthesis
    ScoreAnimation
)

# Add QML files to the project (for IDEs and resource system)
qt_add_qml_module(OrchestrionApp
    URI OrchestrionApp
    VERSION 1.0
    QML_FILES "src/qml/Main.qml"
)

qt_add_resources(RESOURCES resources.qrc)
qt_add_resources(RESOURCES src/App/OrchestrionApp.qrc)

target_sources(OrchestrionApp PRIVATE
    ${RESOURCES}
)

# Add a post-build step to copy SNDFILE_DLL to the output directory
add_custom_command(TARGET OrchestrionApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SNDFILE_DLL} $<TARGET_FILE_DIR:OrchestrionApp>
)

add_subdirectory(MuseScore/share/sound) # installs soundfonts
install(TARGETS OrchestrionApp RUNTIME DESTINATION bin)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/scores/" DESTINATION scores)
if (SNDFILE_DLL)
    install(FILES ${SNDFILE_DLL} DESTINATION bin)
else()
    message(FATAL_ERROR "Could not find sndfile DLL")
endif()
