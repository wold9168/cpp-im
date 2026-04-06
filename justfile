target_dir := "build"

default:
    @just --list --justfile {{ justfile() }}

refresh-deps build_type="Release":
    conan install . --output-folder={{ target_dir }} --build=missing --settings=build_type={{ build_type }}

refresh-bin-tree build_type="Release": (refresh-deps build_type)
    cmake -B {{ target_dir }} -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE={{ build_type }} --fresh

[private]
pre_build_handler build_target="Release": (refresh-bin-tree build_target)

build build_target="Release": (pre_build_handler build_target)
    cmake --build {{ target_dir }}
