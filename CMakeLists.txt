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

include(MuseScore.cmake)

add_subdirectory(src/MusescoreShell)
add_subdirectory(src/OrchestrionNotation)
add_subdirectory(src/OrchestrionShell)

# Add the executable
qt_add_executable(Orchestrion WIN32
    src/main.cpp
    src/app/app.cpp
    src/app/app.h
)

set_target_properties(Orchestrion PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(Orchestrion PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Widgets

    actions
    accessibility
    audio
    braille
    commonscene
    context
    diagnostics
    draw
    engraving
    extensions
    global
    iex_guitarpro
    midi
    mpe
    multiinstances
    orchestrionsequencer
    playback
    project
    shortcuts
    ui
    uicomponents
    workspace

    MusescoreShell
    OrchestrionNotation
    OrchestrionShell
)

# Add QML files to the project (for IDEs and resource system)
qt_add_qml_module(Orchestrion
    URI Orchestrion
    VERSION 1.0
    QML_FILES "src/qml/Main.qml"
)

qt_add_resources(RESOURCES resources.qrc)

target_sources(Orchestrion PRIVATE
    ${RESOURCES}
)

# Add a post-build step to copy SNDFILE_DLL to the output directory
add_custom_command(TARGET Orchestrion POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SNDFILE_DLL} $<TARGET_FILE_DIR:Orchestrion>
)