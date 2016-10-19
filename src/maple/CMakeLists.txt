set(CMAKE_AUTOMOC OFF)

set(SOURCES
    TraceablePacketImpl.cc
    TraceTree.cc
    LoggableTracer.cc
)

add_library(runos_maple STATIC ${SOURCES})

target_link_libraries(runos_maple
       ${GLOG_LIBRARIES}
       Qt5::Core
)