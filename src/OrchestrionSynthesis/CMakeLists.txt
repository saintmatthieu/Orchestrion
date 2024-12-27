declare_module(OrchestrionSynthesis)

set(MODULE_SRC
    internal/SynthesizerConnector.cpp
    internal/SynthesizerConnector.h
    internal/VstTrackAudioInput.cpp
    internal/VstTrackAudioInput.h
    ISynthesizerConnector.h
    OrchestrionSynthesisModule.cpp
    OrchestrionSynthesisModule.h
)

set(MODULE_LINK muse_audio muse_vst vst3sdk::sdk)

setup_module()
