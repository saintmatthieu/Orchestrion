qt_add_executable(touchpad_testapp
    touchpad_testapp_main.cpp
)

set_target_properties(touchpad_testapp PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC OFF
)

qt_add_qml_module(touchpad_testapp
    URI touchpad_testapp
    VERSION 1.0
    QML_FILES touchpad_testapp_main.qml
)

qt_add_resources(RESOURCES touchpad_testapp.qrc)

target_sources(touchpad_testapp
    PRIVATE
        ${RESOURCES}

        touchpad_testapp_model.cpp
        touchpad_testapp_model.h
)

target_link_libraries(touchpad_testapp
    PRIVATE
        Qt6::Core
        Qt6::Quick
        Qt6::Widgets

        muse::global

    PUBLIC
        GestureControllers
)

if (OS_IS_WIN)
    target_link_libraries(touchpad_testapp PRIVATE runtimeobject)
endif()


# target_compile_definitions(touchpad_testapp PRIVATE QT_QML_DEBUG)

# if (MSVC)
#     set_property(TARGET touchpad_testapp PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS --qmljsdebugger=port:3768,block)
# endif()