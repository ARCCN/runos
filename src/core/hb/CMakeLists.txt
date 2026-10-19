find_package(Qt5 5.15.2 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${spdlog_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})

add_library(heartbeatcore STATIC
    heartbeatcore.cc
    heartbeatprotocol.hpp
)

target_link_libraries(heartbeatcore
    PRIVATE
      Qt5::Network
)

set_target_properties(heartbeatcore PROPERTIES
    LANGUAGE CXX
    AUTOMOC ON
    )
