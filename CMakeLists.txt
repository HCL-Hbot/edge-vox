# Root CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(edge_vox VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(EDGE_VOX_BUILD_TESTS "Build tests" ON)
option(EDGE_VOX_BUILD_EXAMPLES "Build examples" ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Add cmake modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Library target
add_library(edge_vox
    src/core/client.cpp
    src/net/rtp_streamer.cpp
    src/net/control_client.cpp
)
target_include_directories(edge_vox 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${MOSQUITTO_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(edge_vox
    PRIVATE
        ${MOSQUITTO_LIBRARIES}
        ${SDL2_LIBRARIES}
)

# Link directories for dependencies
target_link_directories(edge_vox
    PRIVATE
        ${MOSQUITTO_LIBRARY_DIRS}
        ${SDL2_LIBRARY_DIRS}
)

# Installation
include(GNUInstallDirs)
install(TARGETS edge_vox
    EXPORT edge_vox-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    DIRECTORY include/edge_vox
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export targets
install(
    EXPORT edge_vox-targets
    FILE edge_vox-targets.cmake
    # NAMESPACE edge_vox::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/edge_vox
)

# Create and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/edge_vox-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/edge_vox-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/edge_vox
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/edge_vox-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/edge_vox-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/edge_vox-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/edge_vox
)

# Add subdirectories if enabled
if(EDGE_VOX_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(EDGE_VOX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()