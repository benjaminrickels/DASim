add_executable(xpmem-uff-mm-test-server src/server.c)
add_executable(xpmem-uff-mm-test-client src/client.c)

set_target_properties(xpmem-uff-mm-test-server PROPERTIES OUTPUT_NAME "server")
set_target_properties(xpmem-uff-mm-test-client PROPERTIES OUTPUT_NAME "client")

target_include_directories(xpmem-uff-mm-test-server PUBLIC include)
target_include_directories(xpmem-uff-mm-test-client PUBLIC include)

target_link_libraries(xpmem-uff-mm-test-server -lxpmem)
target_link_libraries(xpmem-uff-mm-test-client -lxpmem)
