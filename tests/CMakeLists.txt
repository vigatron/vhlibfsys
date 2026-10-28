cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# VHFile tests - link only to the library target, like external user
add_executable(test_vhfile
    test_vhfile.cpp
)

target_link_libraries(test_vhfile PRIVATE vhlib_fsys)

add_test(NAME vhfile_tests COMMAND test_vhfile)

# VHFSYS tests - link only to the library target, like external user
add_executable(test_vhfsys
    test_vhfsys.cpp
)

target_link_libraries(test_vhfsys PRIVATE vhlib_fsys)

add_test(NAME vhfsys_tests COMMAND test_vhfsys)
