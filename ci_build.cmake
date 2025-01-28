
message(STATUS "CI build")

set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "Source dir")
set(INSTALL_DIR "../build.install" CACHE STRING "Build install dir")
set(INSTALL_SUFFIX "" CACHE STRING "Install suffix")
set(BUILD_NUMBER "12345678" CACHE STRING "Build number")

option(SKIP_RPATH "Skip rpath" OFF)

# CPUS
cmake_host_system_information(RESULT CPUS QUERY NUMBER_OF_LOGICAL_CORES)
if(NOT "${CPUS}" GREATER "0")
    include(ProcessorCount)
    ProcessorCount(CPUS)
endif()


message(STATUS "CPUS=${CPUS}")
message(STATUS "INSTALL_DIR=${INSTALL_DIR}")
message(STATUS "INSTALL_SUFFIX=${INSTALL_SUFFIX}")
message(STATUS "BUILD_NUMBER=${BUILD_NUMBER}")

macro(do_build build_type build_dir)

    set(CONFIGURE_ARGS -GNinja
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
        -DSMTG_ENABLE_VST3_PLUGIN_EXAMPLES=OFF 
        -DSMTG_ENABLE_VST3_HOSTING_EXAMPLES=OFF
    )

    message(STATUS "========= Begin configure =========")
    file(MAKE_DIRECTORY ${build_dir})

    execute_process(
        COMMAND cmake ${SOURCE_DIR} ${CONFIGURE_ARGS} -DCMAKE_BUILD_TYPE=${build_type}
        WORKING_DIRECTORY ${build_dir}
        RESULT_VARIABLE CMAKE_RESULT
    )
    if (CMAKE_RESULT GREATER 0)
        message(FATAL_ERROR "========= Failed configure =========")
    else()
        message(STATUS "========= Success configure =========")
    endif()

    message(STATUS "========= Begin build =========")
    execute_process(
        COMMAND ninja -j ${CPUS}
        WORKING_DIRECTORY ${build_dir}
        RESULT_VARIABLE NINJA_RESULT
    )
    if (NINJA_RESULT GREATER 0)
        message(FATAL_ERROR "========= Failed build =========")
    else()
        message(STATUS "========= Success build =========")
    endif()

endmacro()

macro(do_install build_dir)
    message(STATUS "========= Begin install =========")
    execute_process(
        COMMAND cmake --install ${build_dir}
        RESULT_VARIABLE INSTALL_RESULT
    )
    if (INSTALL_RESULT GREATER 0)
        message(FATAL_ERROR "========= Failed install =========")
    else()
        message(STATUS "========= Success install =========")
    endif()

endmacro()

# Configure and build
set(BUILD_DIR build.release)
do_build(RelWithDebInfo ${BUILD_DIR})
do_install(${BUILD_DIR})
