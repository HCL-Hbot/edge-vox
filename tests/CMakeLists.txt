find_package(GTest REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)

# Create an imported target for Mosquitto
if(NOT TARGET Mosquitto::Mosquitto)
    add_library(Mosquitto::Mosquitto INTERFACE IMPORTED)
    set_target_properties(Mosquitto::Mosquitto PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MOSQUITTO_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${MOSQUITTO_LIBRARIES}"
        INTERFACE_LINK_DIRECTORIES "${MOSQUITTO_LIBRARY_DIRS}"
    )
endif()

# Unit tests
add_executable(unit_tests
    unit/client_test.cpp
    unit/rtp_streamer_test.cpp
    unit/rtp_packet_test.cpp
    unit/packet_buffer_test.cpp
    unit/audio_async_test.cpp
    unit/control_client_test.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        edge_vox
        GTest::GTest
        GTest::Main
        SDL2::SDL2
        Mosquitto::Mosquitto
)

target_include_directories(unit_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src        
)

add_test(NAME unit_tests COMMAND unit_tests)

# Integration tests
add_executable(integration_tests
    integration/streaming_test.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        edge_vox
        GTest::GTest
        GTest::Main
        SDL2::SDL2
        Mosquitto::Mosquitto
)

target_include_directories(integration_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src        
)

add_test(NAME integration_tests COMMAND integration_tests)
