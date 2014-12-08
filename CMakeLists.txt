cmake_minimum_required(VERSION 2.8.11)

project(sdn-zeroconf)

include(third_party/configure.cmake)

# Enable C++11
add_definitions(
    -std=c++11
)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Locally installed libraries
find_package(Qt5Core)
find_package(Qt5Network)
find_package(Boost REQUIRED COMPONENTS graph)
find_package(PkgConfig)
pkg_check_modules(GLOG REQUIRED libglog)

include_directories(
    ${GLOG_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/prefix/include
    ${CMAKE_SOURCE_DIR}/third_party/json11
    ${CMAKE_SOURCE_DIR}/src
)

link_directories(
    ${GLOG_LIBRARY_DIRS}
    ${Boost_LIBRARY_DIRS}
    ${CMAKE_BINARY_DIR}/prefix/lib
)

add_executable(runos
    # Core
    src/Application.cc
    src/Loader.cc
    src/OFMsgUnion.cc
    src/OXMTLVUnion.cc
    src/Packet.cc
    src/Match.cc
    src/TraceTree.cc
    src/Flow.cc
    src/OFTransaction.cc
    src/FluidDump.cc
    # Base
    src/Controller.cc
    src/Switch.cc
    src/LinkDiscovery.cc
    src/Topology.cc
    # Apps
    src/SimpleLearningSwitch.cc
    src/LearningSwitch.cc
    src/CBench.cc
    # Loader
    src/Main.cc
    # Third-party
    third_party/json11/json11.cpp
    # REST
    src/RestListener.cc
    src/Event.cc
    src/AppObject.cc
    src/HostManager.cc
    src/ControllerApp.cc
    src/JsonParser.cc
)

target_link_libraries(runos
    tcmalloc
    Qt5::Core
    Qt5::Network
    tins
    fluid_base
    libfluid_msg.a
    ${GLOG_LIBRARIES}
    ${Boost_LIBRARIES}
    ${QT_QTNETWORK_LIBRARY}
)
