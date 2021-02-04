cmake_minimum_required(VERSION 3.17)
project(swamp_dump C)

set(CMAKE_C_STANDARD 99)


file(GLOB_RECURSE example_src FOLLOW_SYMLINKS
        "*.c"
        )

add_executable(swamp_dump_example
        ${example_src}
        )

target_compile_options(swamp_dump_example PRIVATE -Wall -Wextra -Wshadow -Wstrict-aliasing -ansi -pedantic -Wno-unused-function -Wno-unused-parameter)
target_compile_definitions(swamp_dump_example PRIVATE CONFIGURATION_DEBUG)

target_include_directories(swamp_dump_example PUBLIC include)
target_include_directories(swamp_dump_example PUBLIC ../../deps/clog/src/include)
target_include_directories(swamp_dump_example PUBLIC ../../deps/tiny-libc/src/include)

target_link_libraries(swamp_dump_example PRIVATE swamp_dump)
target_link_libraries(swamp_dump_example PRIVATE m )
