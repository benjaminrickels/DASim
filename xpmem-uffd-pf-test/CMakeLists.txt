add_executable(xpmem-uff-pf-test-server src/server.c)
add_executable(xpmem-uff-pf-test-client src/client.c)

set_target_properties(xpmem-uff-pf-test-server PROPERTIES OUTPUT_NAME "server")
set_target_properties(xpmem-uff-pf-test-client PROPERTIES OUTPUT_NAME "client")

target_include_directories(xpmem-uff-pf-test-server PUBLIC include)
target_include_directories(xpmem-uff-pf-test-client PUBLIC include)

target_link_libraries(xpmem-uff-pf-test-server -lxpmem)
target_link_libraries(xpmem-uff-pf-test-client -lxpmem)
