cmake_minimum_required(VERSION 3.15)

set(TARGET FSM.io)
project(${TARGET} CXX)

# configure C++ standard
set (CMAKE_CXX_STANDARD 20)

# configure conan packages
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Find packages go here.
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)

# Other common choices are STATIC, SHARED, and MODULE
# Output libname matches target name, with the usual extensions on your system
add_library(
    parser_lib STATIC 
    src/parser.cpp 
    include/parser.hpp
)
add_library(
    model_lib STATIC 
    src/model.cpp 
    include/model.hpp
)
add_library(
    fsm_builder_lib STATIC 
    src/FSM_builder.cpp 
    include/FSM_builder.hpp
)


# Adding something we can run - Output name matches target name
# cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
add_executable(
    ${TARGET} 
    src/main.cpp 
    include/FSM_elements.hpp 
    include/observer.hpp
    include/tree.hpp
    include/utility.hpp
)
target_compile_options(${TARGET} PRIVATE -Wall -Werror -Wextra -Wconversion -Wunreachable-code -Wuninitialized -pedantic-errors -Wold-style-cast -Wno-error=unused-variable -Wshadow -Wfloat-equal)

# Make sure you link your targets with this command. It can also link libraries and
# even flags, so linking a target that does not exist will not give a configure-time error.
target_link_libraries(
    ${TARGET} 
    PRIVATE 
        parser_lib
        fsm_builder_lib
        model_lib
        ZLIB::ZLIB
        CURL::libcurl
        ${CONAN_LIBS}
)



