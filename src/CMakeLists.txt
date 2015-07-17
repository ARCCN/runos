set(SOURCES
    # Core
    Application.cc 
    Loader.cc
    OFMsgUnion.cc
    OXMTLVUnion.cc
    Packet.cc
    Match.cc
    TraceTree.cc
    Flow.cc
    OFTransaction.cc
    FluidDump.cc
    # Base
    Controller.cc
    Switch.cc
    LinkDiscovery.cc
    Topology.cc
    # Apps
    SimpleLearningSwitch.cc
    LearningSwitch.cc
    CBench.cc
    Stats.cc
    STP.cc
    # Loader
    Main.cc
    # REST
    RestListener.cc
    Event.cc
    AppObject.cc
    HostManager.cc
    ControllerApp.cc
    WebUIManager.cc
    FlowManager.cc
    # Json
    json11.cpp
)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR} # for Qt-generated moc files
)

add_executable(runos ${SOURCES}) 
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

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
