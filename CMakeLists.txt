####
#set(CMAKE_PREFIX_PATH /usr/local/opt/qt5/lib/cmake/Qt5Core)
####

cmake_minimum_required(VERSION 2.8.11)

project(sdn-zeroconf)

# Enable C++11
set(CMAKE_CXX_FLAGS "-std=c++11")
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Locally installed libraries
include_directories("third_party/prefix/include")
link_directories("third_party/prefix/lib")
# Find the QtWidgets library
find_package(Qt5Core)
# Use pkgconfig
find_package(PkgConfig)
# Find google logging library
pkg_check_modules(GLOG REQUIRED libglog)
include_directories(${GLOG_INCLUDE_DIRS})
link_libraries(${GLOG_LIBRARY_DIRS})

# Add sources
include_directories("src")
add_executable(runos
    # Core
    src/Application.cc
    src/AppProvider.cc
    src/Flow.cc
    src/Controller.cc
    src/OFTransaction.cc
    # Apps
    src/SimpleLearningSwitch.cc
    # Loader
    src/Main.cc
    # Third-party
    third_party/json11/json11.cpp
)

target_link_libraries(runos
    Qt5::Core
    fluid_base fluid_msg
    tins
    ${GLOG_LIBRARIES})
