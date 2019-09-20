find_package(Qt5Network REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src)

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
