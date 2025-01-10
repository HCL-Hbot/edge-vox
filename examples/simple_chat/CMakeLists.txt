add_executable(simple_chat
    main.cpp
)

target_link_libraries(simple_chat
    PRIVATE
        edge_vox
)