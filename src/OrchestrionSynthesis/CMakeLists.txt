declare_module(OrchestrionSynthesis)

set(MODULE_SRC
    internal/AntiMetronomeSynthesizer.cpp
    internal/AntiMetronomeSynthesizer.h
    internal/FluidSynthesizer.cpp
    internal/FluidSynthesizer.h
    internal/IOrchestrionSynthesizer.h
    internal/LowpassFilterBank.cpp
    internal/LowpassFilterBank.h
    internal/LowpassFilteredSynthesizer.cpp
    internal/LowpassFilteredSynthesizer.h
    internal/OrchestrionSynthesizerWrapper.cpp
    internal/OrchestrionSynthesizerWrapper.h
    internal/OrchestrionSynthResolver.cpp
    internal/OrchestrionSynthResolver.h
    internal/OrchestrionVstSynthesizer.cpp
    internal/OrchestrionVstSynthesizer.h
    internal/PolyphonicSynthesizerImpl.cpp
    internal/PolyphonicSynthesizerImpl.h
    internal/PromisedSynthesizer.cpp
    internal/PromisedSynthesizer.h
    internal/SynthesizerConnector.cpp
    internal/SynthesizerConnector.h
    internal/TrackChannelMapper.cpp
    internal/TrackChannelMapper.h
    ISynthesizerConnector.h
    ITrackChannelMapper.h
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
