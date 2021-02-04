cmake_minimum_required(VERSION 3.17)
project(swamp_dump C)

set(CMAKE_C_STANDARD 99)

set(deps ../../deps/)


message("deps ${deps}")

file(GLOB_RECURSE deps_src FOLLOW_SYMLINKS
        "${deps}*/src/lib/*.c"
        )

file(GLOB_RECURSE lib_src FOLLOW_SYMLINKS
        "*.c"
        )

add_library(swamp_dump
    ${deps_src}
    ${lib_src}
)

target_compile_options(swamp_dump PRIVATE -Wall -Wextra -Wshadow -Wstrict-aliasing -ansi -pedantic -Wno-unused-function -Wno-unused-parameter)
target_compile_definitions(swamp_dump PRIVATE CONFIGURATION_DEBUG)

message("include: ${deps}clog/src/include")

target_include_directories(swamp_dump PUBLIC ${deps}runtime-c/src/include)
target_include_directories(swamp_dump PUBLIC ${deps}typeinfo-c/src/include)
target_include_directories(swamp_dump PRIVATE ${deps}tiny-libc/src/include)
target_include_directories(swamp_dump PUBLIC ${deps}flood-c/src/include)
target_include_directories(swamp_dump PRIVATE ${deps}clog/src/include)
target_include_directories(swamp_dump PUBLIC ../include)

target_link_libraries(swamp_dump m)
