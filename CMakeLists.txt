cmake_minimum_required(VERSION 3.31.10)
message("Building with CMake version: ${CMAKE_VERSION}")
project(cpp-im VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS "-Wall -O0 -g -ggdb3")

set(PROJECT_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(PROJECT_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(PROJECT_VERSION_PATCH ${PROJECT_VERSION_PATCH})

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

configure_file(
    ${CMAKE_SOURCE_DIR}/main/include/main/version.h.in
    ${CMAKE_SOURCE_DIR}/main/include/main/version.h
)

FIND_PACKAGE(argparse REQUIRED)
find_package(spdlog REQUIRED)

include(CTest)
enable_testing()

add_subdirectory(main)
#add_subdirectory(other_module)
