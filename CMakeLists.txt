cmake_minimum_required(VERSION 3.16)
project(vhlibfsys VERSION 0.0.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version_vhlibfsys.h"
)

add_library(vhlib_fsys STATIC
    src/vhfile.hpp
    src/vhfile.cpp
    src/vhfsys.hpp
    src/vhfsys.cpp
)

target_include_directories(vhlib_fsys PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)


# 4. Проверяем, создана ли цель vhlib_platform в главном проекте
if(NOT TARGET vhlib_platform)
    # Если мы собираем вашу либу ОТДЕЛЬНО (например, при тестах самой либы),
    # и её нет в главном проекте — только тогда подключаем её локальную копию.
    # Замените '3rdparty/vhlibplatform' на фактический локальный путь, если он есть.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tmp/vhlibplatform/CMakeLists.txt")
        add_subdirectory(tmp/vhlibplatform)
    else()
        message(FATAL_ERROR "vhlib_platform target not found! Please add vhlibplatform to the project.")
    endif()
endif()

# 5. Просто линкуемся с ней. CMake сам найдет уже существующую цель в памяти!
target_link_libraries(vhlib_fsys PUBLIC vhlib_platform)
