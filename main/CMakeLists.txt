get_filename_component(CURRENT_FOLDER_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(artifact "${CURRENT_FOLDER_NAME}")
file(GLOB_RECURSE dir CONFIGURE_DEPENDS src/*.cxx src/*.c src/*.cpp include/*.h include/*.hpp)
add_executable(${artifact} ${dir})
# add_library(${artifact} ${dir})
target_include_directories(${artifact} PUBLIC include)

find_package(argparse REQUIRED)
target_link_libraries(${artifact} PUBLIC
    argparse::argparse
    # other module
)