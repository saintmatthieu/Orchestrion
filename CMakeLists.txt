cmake_minimum_required(VERSION 3.16)

# Set the project name and version
project(HelloQtQuick VERSION 1.0 LANGUAGES CXX)

# Find the Qt6 packages
find_package(Qt6 6.2.4 REQUIRED COMPONENTS Core Gui Qml Quick)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

qt_add_executable(HelloQtQuick WIN32
    src/main.cpp
)

set_target_properties(HelloQtQuick PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_link_libraries(HelloQtQuick PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

# Add QML files to the project (for IDEs and resource system)
qt_add_qml_module(HelloQtQuick
    URI HelloQtQuick
    VERSION 1.0
    QML_FILES "src/qml/Main.qml"
)

set(INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/install")
install(TARGETS HelloQtQuick
    RUNTIME DESTINATION ${INSTALL_DIR}
    BUNDLE DESTINATION ${INSTALL_DIR}
    LIBRARY DESTINATION ${INSTALL_DIR}
)