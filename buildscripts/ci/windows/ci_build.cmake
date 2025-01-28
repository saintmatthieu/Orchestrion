
message(STATUS "Build")

# Config
set(ARTIFACTS_DIR "build.artifacts")
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)

# Build
set(CONFIG
    -DBUILD_TYPE=release_install
    -DBUILD_MODE=${APP_BUILD_MODE}
)

execute_process(
    COMMAND "${ROOT_DIR}/ci_build.bat" ${CONFIG}
    RESULT_VARIABLE BUILD_RESULT
)

if (BUILD_RESULT GREATER 0)
    message(FATAL_ERROR "Failed build")
endif()

