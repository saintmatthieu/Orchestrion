declare_module(Orchestrion)

set(MODULE_SRC
    internal/ComputerKeyboard.cpp
    internal/ComputerKeyboard.h
    internal/ComputerKeyboardMidiController.cpp
    internal/ComputerKeyboardMidiController.h
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
    internal/WebcamBasedIntensityController.cpp
    internal/WebcamBasedIntensityController.h
    IChord.h
    OrchestrionTypes.h
    IComputerKeyboard.h
    IComputerKeyboardMidiController.h
    IIntensityController.h
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
set(MODULE_LINK
    dlib

    muse_audio

    aded.lib
    IlmImfd.lib
    ippicvmt.lib
    ippiwd.lib
    ittnotifyd.lib
    libjpeg-turbod.lib
    libopenjp2d.lib
    libpngd.lib
    libprotobufd.lib
    libtiffd.lib
    opencv_aruco490d.lib
    opencv_bgsegm490d.lib
    opencv_bioinspired490d.lib
    opencv_calib3d490d.lib
    opencv_ccalib490d.lib
    opencv_core490d.lib
    opencv_datasets490d.lib
    opencv_dnn490d.lib
    opencv_dnn_objdetect490d.lib
    opencv_dnn_superres490d.lib
    opencv_dpm490d.lib
    opencv_face490d.lib
    opencv_features2d490d.lib
    opencv_flann490d.lib
    opencv_fuzzy490d.lib
    opencv_gapi490d.lib
    opencv_hfs490d.lib
    opencv_highgui490d.lib
    opencv_imgcodecs490d.lib
    opencv_imgproc490d.lib
    opencv_img_hash490d.lib
    opencv_intensity_transform490d.lib
    opencv_line_descriptor490d.lib
    opencv_mcc490d.lib
    opencv_ml490d.lib
    opencv_objdetect490d.lib
    opencv_optflow490d.lib
    opencv_phase_unwrapping490d.lib
    opencv_photo490d.lib
    opencv_plot490d.lib
    opencv_quality490d.lib
    opencv_rapid490d.lib
    opencv_reg490d.lib
    opencv_rgbd490d.lib
    opencv_saliency490d.lib
    opencv_shape490d.lib
    opencv_signal490d.lib
    opencv_stereo490d.lib
    opencv_stitching490d.lib
    opencv_structured_light490d.lib
    opencv_superres490d.lib
    opencv_surface_matching490d.lib
    opencv_text490d.lib
    opencv_tracking490d.lib
    opencv_video490d.lib
    opencv_videoio490d.lib
    opencv_videostab490d.lib
    opencv_wechat_qrcode490d.lib
    opencv_xfeatures2d490d.lib
    opencv_ximgproc490d.lib
    opencv_xobjdetect490d.lib
    opencv_xphoto490d.lib
    zlibd.lib
)
set(MODULE_INCLUDE PRIVATE "C:/Users/saint/Downloads/MSVC142_64/Debug/include")

# https://github.com/thommyho/Cpp-OpenCV-Windows-PreBuilts

setup_module()
target_link_directories(Orchestrion PUBLIC "C:/Users/saint/Downloads/MSVC142_64/Debug/lib")
