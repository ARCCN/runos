cmake_minimum_required(VERSION 2.8.11)
enable_testing()
project(runos)

include(third_party/configure.cmake)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Locally installed libraries
find_package(Qt5Core)
find_package(Qt5Network)
find_package(Boost REQUIRED COMPONENTS
    system thread unit_test_framework coroutine context)
find_package(PkgConfig)
pkg_check_modules(GLOG REQUIRED libglog)

include_directories(SYSTEM
    ${GLOG_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/prefix/include
    ${CMAKE_SOURCE_DIR}/src
)

link_directories(
    ${GLOG_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS}
    ${CMAKE_BINARY_DIR}/prefix/lib
)

add_subdirectory(src)
add_subdirectory(web)
#add_subdirectory(test)
