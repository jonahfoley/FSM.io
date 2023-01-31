cmake_minimum_required(VERSION 3.15)

project(FSM.io CXX)

# Find packages go here.
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
    
# Other common choices are STATIC, SHARED, and MODULE
# Output libname matches target name, with the usual extensions on your system
add_library(
    test_lib STATIC 
    src/test.cpp 
    include/test.hpp
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
        test_lib
        ZLIB::ZLIB
        curl
)




