declare_module(GestureControllers)

set(TOUCHPAD_SRC
    internal/Touchpad/IClock.h
    internal/Touchpad/IOperatingSystemTouchpad.h
    internal/Touchpad/ITouchpad.h
    internal/Touchpad/OperatingSystemTouchpadFactory.cpp
    internal/Touchpad/OperatingSystemTouchpadFactory.h
    internal/Touchpad/DummyOperatingSystemTouchpad.h
    internal/Touchpad/OperatingSystemTouchpadProcessor.cpp
    internal/Touchpad/OperatingSystemTouchpadProcessor.h
    internal/Touchpad/SteadyClock.h
    internal/Touchpad/SwipeGestureController.cpp
    internal/Touchpad/SwipeGestureController.h
    internal/Touchpad/Touchpad.cpp
    internal/Touchpad/Touchpad.h
    internal/Touchpad/TouchpadGestureController.cpp
    internal/Touchpad/TouchpadGestureController.h
)

if (OS_IS_WIN)
    set(TOUCHPAD_SRC
        ${TOUCHPAD_SRC}
        internal/Touchpad/WindowsTouchpad.cpp
        internal/Touchpad/WindowsTouchpad.h
    )
endif()

set(MODULE_SRC
    internal/ComputerKeyboard/ComputerKeyboard.cpp
    internal/ComputerKeyboard/ComputerKeyboard.h
    internal/ComputerKeyboard/ComputerKeyboardGestureController.cpp
    internal/ComputerKeyboard/ComputerKeyboardGestureController.h

    internal/MidiDevice/MidiDeviceGestureController.cpp
    internal/MidiDevice/MidiDeviceGestureController.h

    ${TOUCHPAD_SRC}

    internal/GestureControllerConfiguration.cpp
    internal/GestureControllerConfiguration.h
    internal/GestureControllerInternalTypes.h
    internal/GestureControllerSelector.cpp
    internal/GestureControllerSelector.h
    internal/IGestureControllerConfiguration.h

    GestureControllersModule.cpp
    GestureControllersModule.h
    GestureControllerTypes.h
    IComputerKeyboard.h
    IGestureController.h
    IGestureControllerSelector.h
    ITouchpadGestureController.h
)

set(MODULE_NOT_LINK_GLOBAL ON)
set(MODULE_USE_PCH_NONE ON)
set(MODULE_USE_UNITY_NONE ON)
set(MODULE_INCLUDE ${MUSE_FRAMEWORK_PATH}/src/framework)
set(MODULE_LINK muse_audio muse_global)
set(MODULE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# If on Windows, add core library dependencies
if (WIN32)
    set(MODULE_LINK ${MODULE_LINK} hid)
endif()

setup_module()

if (OS_IS_WIN)
    # Don't know why I'm getting a
    # `[build] ld: open() failed, errno=21 for 'src/GestureControllers/internal/Touchpad/testapp/touchpad_testapp'`
    # error when building on Mac. Will have to be fixed when we support touchpad controller on mac.

    # Update to Qt 6.9.1: commenting out, getting linkage error I haven't figured out:
    # [build] touchpad_testapp_main.cpp.obj : error LNK2001: unresolved external symbol "public: virtual struct QMetaObject const * __cdecl muse::Invoker::metaObject(void)const " (?metaObject@Invoker@muse@@UEBAPEBUQMetaObject@@XZ)
    # add_subdirectory(internal/Touchpad/testapp)
endif()
