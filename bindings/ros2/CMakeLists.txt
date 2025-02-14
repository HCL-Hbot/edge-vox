cmake_minimum_required(VERSION 3.8)
project(edgevox_ros)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Allow setting EdgeVox root directory via command line or environment
if(NOT DEFINED EDGEVOX_ROOT)
    if(DEFINED ENV{EDGEVOX_ROOT})
        set(EDGEVOX_ROOT $ENV{EDGEVOX_ROOT})
    else()
        # Try to find it relative to this file's location
        get_filename_component(EDGEVOX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
        if(NOT EXISTS "${EDGEVOX_ROOT}/src/audio/audio_async.cpp")
            message(FATAL_ERROR 
                "EdgeVox root directory not found. Please set EDGEVOX_ROOT:\n"
                "Option 1: When running cmake: -DEDGEVOX_ROOT=/path/to/edge-vox\n"
                "Option 2: As environment variable: export EDGEVOX_ROOT=/path/to/edge-vox\n"
                "Current attempted path: ${EDGEVOX_ROOT}")
        endif()
    endif()
endif()

message(STATUS "Using EdgeVox root: ${EDGEVOX_ROOT}")

# include(FetchContent)
# FetchContent_Declare(
#     audio_common_rel_package
#     URL https://github.com/ros-drivers/audio_common.git
#     SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/external/audio_common
# )
# FetchContent_MakeAvailable(audio_common_rel_package)


# Can we make this stuff some sort of external package?like with tensorflow?
# We need to build audio_common_msgs from source.
# <!-- ```bash
# sudo apt-get install ros-jazzy-audio-common-msgs
# ``` -->
# 1. First, get the audio_common_msgs source in your workspace:
# ```bash
# cd ~/ros2_ws/src
# git clone https://github.com/ros-drivers/audio_common.git -b ros2
# ```

# 2. Since audio_common is a meta-package, we only need audio_common_msgs. We can create a more focused link:
# ```bash
# cd ~/ros2_ws/src
# mkdir -p audio_common_msgs
# cp -r audio_common/audio_common_msgs/* audio_common_msgs/
# rm -rf audio_common  # Remove the full repo since we copied what we need
# ```






find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(audio_common_msgs REQUIRED)

# Define source file path
set(AUDIO_ASYNC_SOURCE "${EDGEVOX_ROOT}/src/audio/audio_async.cpp")

# Include directories
include_directories(
  include
  ${EDGEVOX_ROOT}/include
)

# Create the audio capture node
add_executable(audio_capture_node 
  src/audio_capture_node.cpp
  ${AUDIO_ASYNC_SOURCE}
)

target_include_directories(audio_capture_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${EDGEVOX_ROOT}/include
)

# Find SDL2
find_package(SDL2 REQUIRED)

ament_target_dependencies(audio_capture_node
  rclcpp
  std_msgs
  audio_common_msgs
)

target_link_libraries(audio_capture_node
  ${SDL2_LIBRARIES}
)

# Install targets
install(TARGETS
  audio_capture_node
  DESTINATION lib/${PROJECT_NAME}
)

# Install include directories
install(DIRECTORY include/
  DESTINATION include
)

# Install launch and config directories
install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
