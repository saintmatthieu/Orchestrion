declare_module(Orchestrion)

set(MODULE_SRC
    internal/ComputerKeyboard.cpp
    internal/ComputerKeyboard.h
    internal/ComputerKeyboardMidiController.cpp
    internal/ComputerKeyboardMidiController.h
    internal/MuseChord.cpp
    internal/MuseChord.h
    internal/Orchestrion.cpp
    internal/Orchestrion.h
    internal/OrchestrionSequencer.cpp
    internal/OrchestrionSequencer.h
    internal/OrchestrionSequencer.cpp
    internal/OrchestrionSequencer.h
    internal/OrchestrionSequencerActionController.cpp
    internal/OrchestrionSequencerActionController.h
    internal/OrchestrionSequencerFactory.cpp
    internal/OrchestrionSequencerFactory.h
    internal/OrchestrionSequencerUiActions.cpp
    internal/OrchestrionSequencerUiActions.h
    internal/VoiceBlank.cpp
    internal/VoiceBlank.h
    internal/VoiceSequencer.cpp
    internal/VoiceSequencer.h
    OrchestrionTypes.h
    IComputerKeyboard.h
    IComputerKeyboardMidiController.h
    IChord.h
    IOrchestrion.h
    IOrchestrionSequencer.h
    IOrchestrionSequencerUiActions.h
    OrchestrionModule.cpp
    OrchestrionModule.h
)

set(MODULE_NOT_LINK_GLOBAL ON)
set(MODULE_USE_PCH_NONE ON)
set(MODULE_USE_UNITY_NONE ON)
set(MODULE_LINK muse_audio)

setup_module()
