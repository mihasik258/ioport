cmake_minimum_required(VERSION 3.10)
project(io_tool C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_DEBUG "Build debug version" ON)

if(BUILD_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    add_compile_options(-g -O0 -DDEBUG)
else()
    set(CMAKE_BUILD_TYPE Release)
    add_compile_options(-O2 -DNDEBUG)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED readline)

add_executable(io_tool
    src/main.c
    src/io_access.c
    src/command_processor.c
)

target_include_directories(io_tool PRIVATE src)

# Install target
install(TARGETS io_tool DESTINATION bin)

# Additional targets
add_custom_target(distclean
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/*
    COMMENT "Cleaning build directory"
)
