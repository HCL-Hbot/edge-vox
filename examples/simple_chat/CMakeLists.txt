add_executable(simple_chat
    main.cpp
)

target_include_directories(simple_chat
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(simple_chat
    PRIVATE
        edge_vox
        ${MOSQUITTO_LIBRARIES}
        ${SDL2_LIBRARIES}
)