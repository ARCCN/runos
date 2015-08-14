cmake_minimum_required(VERSION 2.8.11)
project(runos)

include(third_party/configure.cmake)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Locally installed libraries
find_package(Qt5Core)
find_package(Qt5Network)
find_package(Boost REQUIRED COMPONENTS graph system thread coroutine context)
find_package(PkgConfig)
pkg_check_modules(GLOG REQUIRED libglog)

include_directories(SYSTEM
    ${GLOG_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/prefix/include
)

link_directories(
    ${GLOG_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS}
    ${CMAKE_BINARY_DIR}/prefix/lib
)

add_subdirectory(src)
add_subdirectory(web)
