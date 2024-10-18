project(
    damemory
    VERSION 0.1
    LANGUAGES CXX)

set(DAMEMORY_SOURCES
    src/damemory.cc
)

add_library(damemory_interface INTERFACE)
target_include_directories(damemory_interface INTERFACE include)

add_library(damemory SHARED ${DAMEMORY_SOURCES})
target_include_directories(damemory PUBLIC include)
target_link_libraries(damemory PRIVATE damanager_interface -lxpmem)
target_compile_definitions(damemory PRIVATE DASIM_LOGLEVEL=DASIM_LOGLEVEL_${DAMEMORY_LOGLEVEL})
