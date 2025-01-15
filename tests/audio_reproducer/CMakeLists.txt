cmake_minimum_required(VERSION 3.14)
project(audio_reproducer)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(SDL2 REQUIRED)

# Add the reproducer executable
add_executable(audio_reproducer reproducer.cpp)

# Add include directories
target_include_directories(audio_reproducer
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../../include    # Path to edge_vox headers
        ${CMAKE_SOURCE_DIR}/../../src        # Path to implementation files
)

# Add the source files
target_sources(audio_reproducer
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../../src/audio/audio_async.cpp
)

# Link libraries
target_link_libraries(audio_reproducer
    PRIVATE
        SDL2::SDL2
)