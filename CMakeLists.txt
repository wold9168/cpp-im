cmake_minimum_required(VERSION 3.5.0)
project(cpp-template-project VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS "-Wall -O0 -g -ggdb3")

include(CTest)
enable_testing()

add_subdirectory(main)
#add_subdirectory(other_module)
