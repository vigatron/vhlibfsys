cmake_minimum_required(VERSION 3.16)
project(vhlibfsys VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MYLIB_BUILD_TESTS "Build unit tests" OFF)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version_vhlibfsys.h"
)

set(VHLIB_FSYS_SOURCES
    src/vhfile.hpp
    src/vhfile.cpp
    src/vhfsys.hpp
    src/vhfsys.cpp
)

if(WIN32)
    list(APPEND VHLIB_FSYS_SOURCES src/vhfile_win.cpp)
else()
    list(APPEND VHLIB_FSYS_SOURCES src/vhfile_unix.cpp)
endif()

add_library(vhlib_fsys STATIC ${VHLIB_FSYS_SOURCES})

target_include_directories(vhlib_fsys PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# Large-file support: off_t/fseeko must be 64-bit on 32-bit POSIX builds.
if(NOT WIN32)
    target_compile_definitions(vhlib_fsys PUBLIC _FILE_OFFSET_BITS=64)
endif()

if(MSVC)
    target_compile_definitions(vhlib_fsys PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

enable_testing()

if(MYLIB_BUILD_TESTS)
    add_subdirectory(tests)
endif()
