declare_module(OrchestrionSequencer)

set(MODULE_SRC
    internal/GestureControllerConfigurator.cpp
    internal/GestureControllerConfigurator.h
    internal/IGestureControllerConfigurator.h
    internal/MuseChord.cpp
    internal/MuseChord.h
    internal/ChordTransitionUtil.cpp
    internal/ChordTransitionUtil.h
    internal/MuseMelodySegment.cpp
    internal/MuseMelodySegment.h
    internal/MuseRest.cpp
    internal/MuseRest.h
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
    internal/OrchestrionTypes.cpp
    internal/VoiceBlank.cpp
    internal/VoiceBlank.h
    internal/VoiceSequencer.cpp
    internal/VoiceSequencer.h

    view/GestureControllerSelectionModel.cpp
    view/GestureControllerSelectionModel.h

    IChord.h
    OrchestrionTypes.h
    IMelodySegment.h
    IOrchestrion.h
    IOrchestrionSequencer.h
    IOrchestrionSequencerUiActions.h
    IRest.h
    OrchestrionModule.cpp
    OrchestrionModule.h
)

set(MODULE_NOT_LINK_GLOBAL ON)
set(MODULE_USE_PCH_NONE ON)
set(MODULE_USE_UNITY_NONE ON)
set(MODULE_LINK muse_audio)
set(MODULE_QRC OrchestrionSequencer.qrc)
set(MODULE_QML_IMPORT ${CMAKE_CURRENT_LIST_DIR}/qml)

setup_module()
