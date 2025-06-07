declare_module(ExternalDevices)

set(MODULE_SRC
    internal/AudioDevice/AudioDeviceService.cpp
    internal/AudioDevice/AudioDeviceService.h

    internal/MidiDevice/MidiDeviceService.cpp
    internal/MidiDevice/MidiDeviceService.h

    internal/ExternalDevicesConfiguration.cpp
    internal/ExternalDevicesConfiguration.h
    internal/ExternalDevicesTypes.cpp

    ExternalDevicesModule.cpp
    ExternalDevicesModule.h
    ExternalDevicesTypes.h
    IMidiDeviceService.h
)

set(MODULE_LINK muse_audio)

setup_module()
