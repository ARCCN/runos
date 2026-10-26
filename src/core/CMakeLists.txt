add_subdirectory(core)

find_package(Qt5 5.15.2 REQUIRED)
find_package(cpp-netlib REQUIRED)
find_package(libtins REQUIRED)
find_package(tiny-process-library REQUIRED)
find_package(range-v3 REQUIRED)
find_package(fmt 10.2.1 REQUIRED)
find_package(spdlog REQUIRED)
find_package(Backward REQUIRED)

find_package(libfluid_base CONFIG REQUIRED)
find_package(libfluid_msg CONFIG REQUIRED)
find_package(Libevent REQUIRED)
find_package(editline REQUIRED)

find_path(range-v3_INCLUDE_DIRS
    NAMES meta/meta.hpp range/v3/all.hpp
)

add_library(runos STATIC
    # API
    api/OFConnection.hpp
    api/Port.hpp
    api/Statistics.hpp
    api/Switch.hpp
    api/OFDriver.hpp
    api/FunctionalTraits.hpp
    
    # Base
    lib/ethaddr.cc
    lib/ipv4addr.cc
    lib/pipe_exec.cc
    Application.cc
    Application.hpp
    Controller.cc
    Controller.hpp
    FluidOXMAdapter.cc
    FluidOXMAdapter.hpp
    Loader.cc
    Loader.hpp
    IdGen.cc
    
    # lib
    lib/action_parsing.cc
    lib/action_parsing.hpp
    lib/poller.cc
    lib/poller.hpp

    LinkDiscoveryDriver.cc
    LinkDiscoveryDriver.hpp
    Logger.cc
    Logger.hpp
    OFAgentImpl.cc
    OFAgentImpl.hpp
    OFServer.cc
    OFServer.hpp
    PacketParser.cc
    PacketParser.hpp
    PortImpl.cc
    PortImpl.hpp
    StatsBucket.cc
    StatsBucket.hpp
    PropertySheet.cc
    Utility.cc
    ResourceLocator.cc
    DeviceDb.cc

    # Core Apps
    Config.hpp
    Config.cc
    DatabaseConnector.cc
    DatabaseConnector.hpp
    FlowEntriesVerifier.cc
    FlowEntriesVerifier.hpp
    LinkDiscovery.cc
    LinkDiscovery.hpp
    OFMsgSender.cc
    OFMsgSender.hpp
    StatsRulesManager.cc
    StatsRulesManager.hpp
    SwitchImpl.cc
    SwitchImpl.hpp
    SwitchManager.cc
    SwitchManager.hpp
    SwitchOrdering.cc
    SwitchOrdering.hpp
    Topology.cc
    Topology.hpp
    DpidChecker.cc
    DpidChecker.hpp

    # HA
    lib/better_enum.hpp
    Recovery.cc
    Recovery.hpp
    RecoveryModeChecker.cc
    RecoveryModeChecker.hpp
)

add_library(runos_cli STATIC
    CommandLine.cc
    CommandLine.hpp
    LinkDiscoveryCli.cc
    OFServerCli.cc
    SwitchManagerCli.cc
)

add_library(runos_rest STATIC
    
    LinkDiscoveryRest.cc
    OFServerRest.cc
    RecoveryRest.cc
    RestListener.cc
    RestListener.hpp
    StatsBucketRest.cc
    StatsRulesManagerRest.cc
    SwitchManagerRest.cc
    TopologyRest.cc
    FlowTableRest.cc
    GroupTableRest.cc
    MeterTableRest.cc
    AuxDevicesRest.cc
)

# Instruct CMake to run moc automatically when needed.
target_include_directories(runos SYSTEM
    PUBLIC
      ${range-v3_INCLUDE_DIRS}
      ${libfluid_base_INCLUDE_DIRS}
      ${libfluid_msg_INCLUDE_DIRS}
      ${Libevent_INCLUDE_DIRS}
      ${spdlog_INCLUDE_DIRS}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR} # for Qt-generated moc files
      ${libtins_INCLUDE_DIRS}
      ${fmt_INCLUDE_DIRS}
      ${spdlog_INCLUDE_DIRS}
      ${Backward_INCLUDE_DIRS}
      ${tiny-process-library_INCLUDE_DIRS}
    )

target_include_directories(runos_cli SYSTEM
    PRIVATE
      ${editline_INCLUDE_DIRS}
    )

target_include_directories(runos_rest SYSTEM
    PUBLIC
      ${cpp-netlib_INCLUDE_DIRS}
    )

target_link_libraries(runos
    PUBLIC
      Qt5::Core
      runos_core
      ${libfluid_msg_LIB_DIRS_RELEASE}/libfluid_msg.a
      ${libfluid_base_LIB_DIRS_RELEASE}/libfluid_base.a
      ${Libevent_LIBRARIES}
    PRIVATE
      ${libtins_LIBRARIES}
      ${tiny-process-library_LIB_DIRS_RELEASE}/libtiny-process-library.a
      heartbeatcore
      redisdb
      cpqd_print
    )

target_link_libraries(runos_cli
    PRIVATE
      editline::editline
      runos
    )

target_link_libraries(runos_rest
    PRIVATE
      runos
      ${cpp-netlib_LIBRARIES}
    )

set_target_properties(runos PROPERTIES
    LANGUAGE CXX
    AUTOMOC ON
    )

add_subdirectory(hb)
add_subdirectory(redisdb)
add_subdirectory(oflog)
