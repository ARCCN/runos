################################################################################
# Bootstrap
################################################################################

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
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
option(RUNOS_ENABLE_CRASH_REPORTER "Enable crash reporter" ON)
option(RUNOS_DISABLE_ASSERTIONS "Disable RUNOS_ASSERTs" OFF)
option(RUNOS_DISABLE_CRASH_REPORTING "Disable crash repoter" ON)
option(RUNOS_DISABLE_EXPRESSION_DECOMPOSER "Don't decompose expressions in asserts" OFF)

################################################################################
# Build config
################################################################################
find_package(Boost)
include_directories(
      ${Boost_INCLUDE_DIR}
    )


include(CheckIncludeFileCXX)

option(RUNOS_USE_STD_EXPERIMENTAL_SOURCE_LOCATION "Use std::experimental::source_location if available" OFF)
CHECK_INCLUDE_FILE_CXX(experimental/source_location RUNOS_HAVE_STD_EXPERIMENTAL_SOURCE_LOCATION)
CHECK_INCLUDE_FILE_CXX(source_location RUNOS_HAVE_STD_SOURCE_LOCATION)

configure_file(${CMAKE_MODULE_PATH}/build-config.hpp.in
               ${CMAKE_BINARY_DIR}/include/runos/build-config.hpp)
include_directories(${CMAKE_BINARY_DIR}/include
                    ${CMAKE_SOURCE_DIR}/src
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

# target_link_libraries(runos-bin runos ${RUNOS_CLI} ${RUNOS_REST})
execute_process(
    COMMAND bash -c "find /nix -name 'libstdc++.so.6' | grep gcc-14 | head -n 1"
    OUTPUT_VARIABLE PATH_LIBSTDC
)

string(STRIP ${PATH_LIBSTDC} PATH_LIBSTDC_SO)
target_link_libraries(runos-bin runos ${RUNOS_CLI} ${RUNOS_REST} ${PATH_LIBSTDC_SO})


set_target_properties(runos-bin PROPERTIES OUTPUT_NAME runos)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
