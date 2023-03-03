add_executable(23_list_01 23_list_01.cc)
target_link_libraries(23_list_01 workflow)

add_executable(23_list_01_init 23_list_01_init.cc)
target_link_libraries(23_list_01_init workflow spdlog)

