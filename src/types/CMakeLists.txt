set(CMAKE_AUTOMOC OFF)

set(SOURCES
    ethaddr.cc
    exception.cc
    IPv6Addr.cc
)

add_library(runos_types STATIC ${SOURCES})

target_link_libraries(runos_types ${Boost_UNIT_TEST_FRAMEWORK})
