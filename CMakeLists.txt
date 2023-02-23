cmake_minimum_required(VERSION 3.15)

set(TARGET FSM.io)
project(${TARGET} CXX)

# configure C++ standard
set (CMAKE_CXX_STANDARD 20)
set(CONAN_SYSTEM_INCLUDES ON)

# use fmtlib in header only mode
add_definitions(-DFMT_HEADER_ONLY)

# configure conan packages - install conan if necessary
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.10/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Other common choices are STATIC, SHARED, and MODULE
# Output libname matches target name, with the usual extensions on your system
add_library(
    parser_lib STATIC 
    src/parser.cpp 
    include/parser.hpp
)
add_library(
    fsm_builder_lib STATIC 
    src/FSM_builder.cpp 
    include/FSM_builder.hpp
)
add_library(
    transition_matrix_lib STATIC 
    src/transition_matrix.cpp 
    include/transition_matrix.hpp
)
add_library(
    app_lib STATIC 
    src/app.cpp 
    include/app.hpp
)


# Adding something we can run - Output name matches target name
# cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
add_executable(
    ${TARGET} 
    src/main.cpp 
    include/utility.hpp
    include/observer.hpp
    include/tree.hpp
    include/ranges_helpers.hpp
    include/FSM_elements.hpp
)

target_compile_options(
    ${TARGET} PRIVATE
    -Werror
    -Wpedantic
    -Wall
    -Wextra
    -Wdouble-promotion
    -Wformat=2
    -Wformat-overflow=2
    -Wformat-signedness
    -Wformat-truncation=2
    -Wnull-dereference
    -Wimplicit-fallthrough=5
    -Wmissing-include-dirs
    -Wshift-overflow=2
    -Wswitch-enum
    -Wsync-nand
    -Wunused-parameter
    -Wunused-const-variable=1
    -Wunknown-pragmas
    -Wstringop-overflow=4
    -Wsuggest-override
    -Walloc-zero
    -Walloca
    -Warray-bounds=2
    -Wduplicated-branches
    -Wduplicated-cond
    -Wtrampolines
    -Wfloat-equal
    -Wshadow
    -Waligned-new=all
    -Wplacement-new=2
    -Wundef
    -Wunused-macros
    -Wcast-qual
    -Wcast-align
    -Wconditionally-supported
    -Wconversion
    -Wzero-as-null-pointer-constant
    -Wdate-time
    -Wuseless-cast
    -Wsign-conversion
    -Wsized-deallocation
    -Wlogical-op
    -Wmissing-declarations
    -Wnormalized=id
    -Wpacked
    -Wredundant-decls
    -Wrestrict
    -Winvalid-pch
    -Wvector-operation-performance
    -Wdisabled-optimization
    -Wstack-protector
# Additional warnings for C++ dialect
    -Wctor-dtor-privacy
    -Wnoexcept
    -Wnon-virtual-dtor
    -Wstrict-null-sentinel
    -Wold-style-cast
    -Woverloaded-virtual
    -Wsign-promo
    -Wunreachable-code 
    -Wuninitialized 
    -pedantic-errors 
    -Wold-style-cast 
    -Wno-error=unused-variable 
    -Wshadow
)

# Make sure you link your targets with this command. It can also link libraries and
# even flags, so linking a target that does not exist will not give a configure-time error.
target_link_libraries(
    ${TARGET} 
    PRIVATE 
        app_lib
        parser_lib
        fsm_builder_lib
        transition_matrix_lib
        ${CONAN_LIBS}
)



