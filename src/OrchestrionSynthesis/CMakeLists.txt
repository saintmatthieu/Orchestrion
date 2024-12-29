declare_module(OrchestrionSynthesis)

set(MODULE_SRC
    internal/FluidTrackAudioInput.cpp
    internal/FluidTrackAudioInput.h
    internal/SynthesizerConnector.cpp
    internal/SynthesizerConnector.h
    internal/TrackAudioInput.cpp
    internal/TrackAudioInput.h
    internal/VstTrackAudioInput.cpp
    internal/VstTrackAudioInput.h
    ISynthesizerConnector.h
    OrchestrionSynthesisModule.cpp
    OrchestrionSynthesisModule.h
)

set(FLUIDSYNTH_DIR ${PROJECT_SOURCE_DIR}/MuseScore/src/framework/audio/thirdparty/fluidsynth/fluidsynth-2.3.3)
set (FLUIDSYNTH_INC
    ${FLUIDSYNTH_DIR}/include
    ${FLUIDSYNTH_DIR}/src
    ${FLUIDSYNTH_DIR}/src/external
    ${FLUIDSYNTH_DIR}/src/utils
    ${FLUIDSYNTH_DIR}/src/midi
    ${FLUIDSYNTH_DIR}/src/rvoice
    ${FLUIDSYNTH_DIR}/src/sfloader
    ${FLUIDSYNTH_DIR}/src/bindings
    ${FLUIDSYNTH_DIR}/src/synth
    ${FLUIDSYNTH_DIR}/src/drivers
    )

set (MODULE_INCLUDE
    ${FLUIDSYNTH_INC}
    ${PROJECT_SOURCE_DIR}/MuseScore/src/framework/audio/internal/synthesizers/fluidsynth
    )

set(MODULE_LINK
    muse_audio
    muse_vst
    vst3sdk::sdk
    fluidsynth
    DSPFilters::DSPFilters
)

setup_module()
