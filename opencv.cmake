include(FetchContent)

# OpenCV
FetchContent_Declare(
  opencv
  GIT_REPOSITORY https://github.com/opencv/opencv.git
  GIT_TAG        "4.11.0"
)

FetchContent_MakeAvailable(opencv)
