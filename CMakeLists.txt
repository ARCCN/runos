################################################################################
# Bootstrap
################################################################################

cmake_minimum_required(VERSION 3.26 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
project(Runos C CXX)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
        "MinSizeRel" "RelWithDebInfo")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-cpp")
endif()

################################################################################
# Options
################################################################################
option(RUNOS_ENABLE_MEMCHECK "Run tests under valgrind" OFF)
option(RUNOS_ENABLE_TESTSING "Enable unit tests" OFF)
option(RUNOS_ENABLE_REST_API "Enable REST API" ON)
option(RUNOS_ENABLE_CLI "Enable command line interface" ON)
option(RUNOS_ENABLE_CRASH_REPORTER "Enable crash reporter" OFF)
option(RUNOS_DISABLE_ASSERTIONS "Disable RUNOS_ASSERTs" OFF)
option(RUNOS_DISABLE_CRASH_REPORTING "Disable crash repoter" ON)
option(RUNOS_DISABLE_EXPRESSION_DECOMPOSER "Don't decompose expressions in asserts" OFF)

################################################################################
# Build config
################################################################################

include(CheckIncludeFileCXX)

option(RUNOS_USE_STD_EXPERIMENTAL_SOURCE_LOCATION "Use std::experimental::source_location if available" OFF)

configure_file(cmake/build-config.hpp.in
                "${CMAKE_BINARY_DIR}/include/runos/build-config.hpp")

find_package(fmt 10.2.1 REQUIRED)

include_directories(${CMAKE_BINARY_DIR}/include
                    ${CMAKE_SOURCE_DIR}/src
                    ${fmt_INCLUDE_DIRS}
                    )

################################################################################
# Custom functions
################################################################################
if (RUNOS_ENABLE_MEMCHECK)
    find_package(Valgrind REQUIRED)
    function(runos_add_test name)
        add_test(${name} ${Valgrind_EXECUTABLE} --leak-check=full --error-exitcode=1 ${ARGN})
    endfunction()
else()
    function(runos_add_test name)
        add_test(${name} ${ARGN})
    endfunction()
endif()

################################################################################
# Subdirectories
################################################################################
add_subdirectory(src)
add_subdirectory(web)

################################################################################
# Target
################################################################################
add_executable(runos-bin src/core/Main.cc) 

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if (RUNOS_ENABLE_CLI)
        set(RUNOS_CLI -Wl,--whole-archive runos_cli -Wl,--no-whole-archive)
    endif()
    if (RUNOS_ENABLE_REST_API)
        set(RUNOS_REST -Wl,--whole-archive runos_rest -Wl,--no-whole-archive)
    endif()
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    if (RUNOS_ENABLE_CLI)
        set(RUNOS_CLI -all_load runos_cli)
    endif()
    if (RUNOS_ENABLE_REST_API)
        set(RUNOS_REST -all_load runos_rest)
    endif()
else()
    message(STATUS "Your operating system is not supported")
endif()


target_link_libraries(runos-bin runos ${RUNOS_CLI} ${RUNOS_REST})


set_target_properties(runos-bin PROPERTIES OUTPUT_NAME runos)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

################################################################################
# Tests
################################################################################
if(RUNOS_ENABLE_TESTSING)
    enable_testing()
    find_program(PYTEST pytest)
    if(PYTEST)
        add_test(NAME smoke
            COMMAND ${PYTEST} -v -x ${CMAKE_SOURCE_DIR}/tests/test_smoke.py
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    else()
        message(WARNING "pytest not found; smoke tests disabled")
    endif()
endif()
