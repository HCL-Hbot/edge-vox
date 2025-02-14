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

# Rest of your CMake file...
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
