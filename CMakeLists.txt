cmake_minimum_required(VERSION 3.15)
project(edge-vox VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Library target
add_library(edge_vox)
target_include_directories(edge_vox 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Organize source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
target_sources(edge_vox PRIVATE ${SOURCES})

# Tests (optional)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    option(BUILD_TESTS "Build tests" ON)
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(tests)
    endif()
endif()

# Examples
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    option(BUILD_EXAMPLES "Build examples" ON)
    if(BUILD_EXAMPLES)
        add_subdirectory(examples)
    endif()
endif()