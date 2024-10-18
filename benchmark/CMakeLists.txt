add_executable(benchmark-benchmark src/benchmark.cc src/common.cc)
add_executable(benchmark-client src/client.cc src/common.cc)

set_target_properties(benchmark-benchmark PROPERTIES OUTPUT_NAME "benchmark")
set_target_properties(benchmark-client PROPERTIES OUTPUT_NAME "client")

target_include_directories(benchmark-benchmark PUBLIC include)
target_include_directories(benchmark-client PUBLIC include)

target_link_libraries(benchmark-benchmark -lxpmem)
target_link_libraries(benchmark-client -lxpmem)
