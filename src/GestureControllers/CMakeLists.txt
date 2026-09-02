declare_module(GestureControllers)

set(MODULE_SRC
    internal/ComputerKeyboard/ComputerKeyboard.cpp
    internal/ComputerKeyboard/ComputerKeyboard.h
    internal/ComputerKeyboard/ComputerKeyboardGestureController.cpp
    internal/ComputerKeyboard/ComputerKeyboardGestureController.h

    internal/MidiDevice/MidiDeviceGestureController.cpp
    internal/MidiDevice/MidiDeviceGestureController.h

    internal/GestureInput.cpp
    internal/GestureInput.h

    GestureControllersModule.cpp
    GestureControllersModule.h
    IComputerKeyboard.h
    IGestureController.h
    IGestureInput.h
)

set(MODULE_USE_PCH OFF)
set(MODULE_USE_UNITY OFF)
set(MODULE_LINK muse_audio)
set(MODULE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

setup_module()
