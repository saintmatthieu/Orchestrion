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
    internal/InputThread.cpp
    internal/InputThread.h

    GestureControllersModule.cpp
    GestureControllersModule.h
    IComputerKeyboard.h
    IGestureController.h
    IGestureInput.h
)

set(MODULE_USE_PCH OFF)
set(MODULE_USE_UNITY OFF)
set(MODULE_LINK muse_audio)

if (OS_IS_LIN)
  # The keys read off the GUI thread, from X11 (XInput2's raw events)
  list(APPEND MODULE_SRC
    internal/ComputerKeyboard/RawKeyboard_lin.cpp
    internal/ComputerKeyboard/RawKeyboard.h
  )
  list(APPEND MODULE_LINK X11 Xi)
elseif (OS_IS_WIN)
  # The keys read off the GUI thread, from Raw Input
  list(APPEND MODULE_SRC
    internal/ComputerKeyboard/RawKeyboard_win.cpp
    internal/ComputerKeyboard/RawKeyboard.h
  )
endif()
set(MODULE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

setup_module()
