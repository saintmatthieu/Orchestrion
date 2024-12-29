include(FetchContent)

FetchContent_Declare(
  DSPFilters
  GIT_REPOSITORY https://github.com/vinniefalco/DSPFilters.git
  GIT_TAG        "acc49170e79a94fcb9c04b8a2116e9f8dffd1c7d"
)

FetchContent_MakeAvailable(DSPFilters)

if (MSVC)
    add_compile_options(/FI${PROJECT_SOURCE_DIR}/src/OrchestrionSynthesis/internal/DSPFilters_tr1_fix.hpp)
endif()

add_subdirectory(${dspfilters_SOURCE_DIR}/shared/DSPFilters)
add_library(DSPFilters::DSPFilters ALIAS DSPFilters)
