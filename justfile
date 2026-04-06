target_dir := "build"

default:
    @just --list --justfile {{justfile()}}

refresh-deps:
    conan install . --output-folder={{ target_dir }} --build=missing
    cmake -B {{ target_dir }} -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug --fresh

build: refresh-deps
    cmake --build {{ target_dir }}
