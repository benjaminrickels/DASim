cmake_minimum_required(VERSION 3.1...3.27)
project(
    dasim
    VERSION 0.1
    LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DAMEMORY_LOGLEVEL INFO CACHE STRING
    "Log level of the damemory library: {TRACE,DEBUG,INFO,WARNING,ERROR,FATAL}")

set(DAMANAGER_LOGLEVEL INFO CACHE STRING
    "Log level of damanager: {TRACE,DEBUG,INFO,WARNING,ERROR,FATAL}")

add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(damemory)
add_subdirectory(damanager)

add_subdirectory(benchmark)

add_subdirectory(xpmem-uffd-mm-test)
add_subdirectory(xpmem-uffd-pf-test)

add_subdirectory(test)
add_subdirectory(demo)
