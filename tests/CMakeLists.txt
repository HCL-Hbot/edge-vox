find_package(GTest REQUIRED)

# Unit tests
add_executable(unit_tests
    unit/client_test.cpp
    unit/rtp_streamer_test.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        edge_vox
        GTest::GTest
        GTest::Main
        SDL2::SDL2
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
)

add_test(NAME integration_tests COMMAND integration_tests)