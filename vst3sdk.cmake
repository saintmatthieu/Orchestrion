# Use FetchContent to download the VST3 SDK
include(FetchContent)

FetchContent_Declare(
  vst3sdk
  GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
  GIT_TAG        "v3.7.12_build_20"
)

FetchContent_MakeAvailable(vst3sdk)

set(MUSE_MODULE_VST_VST3_SDK_PATH ${vst3sdk_SOURCE_DIR})
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ADD_VST3_HOSTING_SAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)

add_library(vst3sdk::sdk ALIAS sdk)
add_library(vst3sdk::sdk_hosting ALIAS sdk_hosting)
