declare_module(OrchestrionSynthesis)

# Vendored DSP (see thirdparty/README.md): kept out of the unity build and the
# precompiled header, as foreign code.
set(THIRDPARTY_DSP_SRC
    thirdparty/DynamicRangeProcessor/CompressorProcessor.cpp
    thirdparty/DynamicRangeProcessor/CompressorProcessor.h
    thirdparty/DynamicRangeProcessor/DynamicRangeProcessorTypes.h
    thirdparty/DynamicRangeProcessor/MathApprox.h
    thirdparty/SimpleCompressor/GainReductionComputer.cpp
    thirdparty/SimpleCompressor/GainReductionComputer.h
    thirdparty/SimpleCompressor/LookAheadGainReduction.cpp
    thirdparty/SimpleCompressor/LookAheadGainReduction.h
)
set_source_files_properties(${THIRDPARTY_DSP_SRC} PROPERTIES
    SKIP_UNITY_BUILD_INCLUSION ON
    SKIP_PRECOMPILE_HEADERS ON
)

set(MODULE_SRC
    internal/AntiMetronomeSynthesizer.cpp
    internal/AntiMetronomeSynthesizer.h
    internal/BuiltInEffectResources.h
    internal/BuiltInEffectRuntime.h
    internal/BuiltInEffects.cpp
    internal/BuiltInEffects.h
    internal/DynamicsProcessor.cpp
    internal/DynamicsProcessor.h
    internal/ParameterStore.h
    internal/ReverbPresetParameters.h
    internal/ReverbEffect.cpp
    internal/ReverbEffect.h
    internal/EffectChain.cpp
    internal/EffectChain.h
    internal/FluidSynthesizer.cpp
    internal/FluidSynthesizer.h
    internal/IOrchestrionSynthesizer.h
    internal/LowpassFilterBank.cpp
    internal/LowpassFilterBank.h
    internal/LowpassFilteredSynthesizer.cpp
    internal/LowpassFilteredSynthesizer.h
    internal/OrchestrionSynthesizerWrapper.cpp
    internal/OrchestrionSynthesizerWrapper.h
    internal/OrchestrionSynthesisConfiguration.cpp
    internal/OrchestrionSynthesisConfiguration.h
    internal/OrchestrionSynthResolver.cpp
    internal/OrchestrionSynthResolver.h
    internal/OrchestrionFxResolver.cpp
    internal/OrchestrionFxResolver.h
    internal/OrchestrionVstSynthesizer.cpp
    internal/OrchestrionVstSynthesizer.h
    internal/PolyphonicSynthesizerImpl.cpp
    internal/PolyphonicSynthesizerImpl.h
    internal/PromisedSynthesizer.cpp
    internal/PromisedSynthesizer.h
    internal/SynthesizerConnector.cpp
    internal/SynthesizerConnector.h
    internal/SynthesizerManager.cpp
    internal/SynthesizerManager.h
    internal/TrackChannelMapper.cpp
    internal/TrackChannelMapper.h
    BuiltInEffectTypes.h
    IBuiltInEffects.h
    IEffectChain.h
    ReverbPresets.h
    ITrackChannelMapper.h
    ISynthesizerConnector.h
    ISynthesizerManager.h
    OrchestrionSynthesisModule.cpp
    OrchestrionSynthesisModule.h
    view/EffectMeterModel.cpp
    view/EffectMeterModel.h
    view/EffectHeaderModel.cpp
    view/EffectHeaderModel.h
    ${THIRDPARTY_DSP_SRC}
)

set(FLUIDSYNTH_DIR ${MUSE_FRAMEWORK_SRC_PATH}/audio/thirdparty/fluidsynth/fluidsynth-2.3.3)
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
    ${MUSE_FRAMEWORK_SRC_PATH}/audio
    ${MUSE_FRAMEWORK_SRC_PATH}/audio/engine/internal/synthesizers/fluidsynth
    ${vst3sdk_SOURCE_DIR}
    )

set(MODULE_LINK
    muse_audio
    muse_vst
    vst3sdk
    fluidsynth
    DSPFilters
)

setup_module()
