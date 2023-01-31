cmake_minimum_required(VERSION 3.15)

project(FSM.io CXX)

# configure C++ standard
set (CMAKE_CXX_STANDARD 20)

# configure VCPKG
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_TOOLCHAIN_FILE "/home/jonah/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_TARGET_TRIPLET "x64-linux")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(VCPKG_TARGET_TRIPLET "x64-windows")
endif()

# Find packages go here.
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(tl-expected REQUIRED)

# Other common choices are STATIC, SHARED, and MODULE
# Output libname matches target name, with the usual extensions on your system
add_library(
    parser_lib STATIC 
    src/parser.cpp 
    include/parser.hpp
)

# warnings configuration
set(DESIRED_WARNINGS "-Wall -Wextra -Wconversion -Wunreachable-code -Wuninitialized -pedantic-errors -Wold-style-cast -Wno-error=unused-variable -Wshadow -Wfloat-equal")

# Adding something we can run - Output name matches target name
# cmake .. -G"Unix Makefiles"
set(SOURCES 
    src/main.cpp 
)
add_executable(FSM.io ${SOURCES})
target_compile_features(FSM.io PRIVATE cxx_std_20)

# Make sure you link your targets with this command. It can also link libraries and
# even flags, so linking a target that does not exist will not give a configure-time error.
target_link_libraries(
    FSM.io 
    PRIVATE 
        parser_lib
        ZLIB::ZLIB
        CURL::libcurl
        fmt::fmt-header-only
        tl::expected
)




