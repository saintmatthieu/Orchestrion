# Runs *after* all upstream subdirectory installs have executed, so we can
# strip dev-only artifacts that vendored projects (currently KDDockWidgets,
# vendored via MuseScore/src/framework/dockwindow/thirdparty) drop into the
# install tree.
#
# add_subdirectory() this from the *very end* of the top-level CMakeLists.txt
# so the generated cmake_install.cmake includes us last.

if(NOT OS_IS_WIN)
    return()
endif()

install(CODE [[
    file(REMOVE_RECURSE
        "${CMAKE_INSTALL_PREFIX}/include/kddockwidgets-qt6"
        "${CMAKE_INSTALL_PREFIX}/lib/cmake/KDDockWidgets-qt6"
    )
    file(GLOB _kddw_libs "${CMAKE_INSTALL_PREFIX}/lib/kddockwidgets-qt6*.lib")
    if(_kddw_libs)
        file(REMOVE ${_kddw_libs})
    endif()

    foreach(_dir
        "${CMAKE_INSTALL_PREFIX}/include"
        "${CMAKE_INSTALL_PREFIX}/lib/cmake"
        "${CMAKE_INSTALL_PREFIX}/lib")
        if(IS_DIRECTORY "${_dir}")
            file(GLOB _leftover "${_dir}/*")
            if(NOT _leftover)
                file(REMOVE_RECURSE "${_dir}")
            endif()
        endif()
    endforeach()
]])
