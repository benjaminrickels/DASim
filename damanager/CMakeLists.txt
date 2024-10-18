project(
    damanager
    VERSION 0.1
    LANGUAGES CXX)

set(DAMANAGER_SOURCES
    src/main.cc
    src/buffer_manager.cc
    src/client_manager.cc
    src/client.cc
    src/demo_buffer_manager.cc
    src/util.cc
)

add_library(damanager_interface INTERFACE)
target_include_directories(damanager_interface INTERFACE include)

add_executable(damanager ${DAMANAGER_SOURCES})
target_include_directories(damanager PRIVATE include)
target_link_libraries(damanager PRIVATE damemory_interface -lxpmem)
target_compile_definitions(damanager PRIVATE DASIM_LOGLEVEL=DASIM_LOGLEVEL_${DAMANAGER_LOGLEVEL})
