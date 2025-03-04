include(FetchContent)

FetchContent_Declare(
  dlib
  GIT_REPOSITORY https://github.com/davisking/dlib.git
  GIT_TAG        "v19.24.8"
)

FetchContent_MakeAvailable(dlib)
