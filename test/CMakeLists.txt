add_executable(test1 test1.cc)

add_executable(test2_1 test2_1.cc)
add_executable(test2_2 test2_2.cc)
add_executable(test2_3 test2_3.cc)

target_link_libraries(test1 PRIVATE damemory)

target_link_libraries(test2_1 PRIVATE damemory)
target_link_libraries(test2_2 PRIVATE damemory)
target_link_libraries(test2_3 PRIVATE damemory)
