add_subdirectory(core)

find_package(PkgConfig REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(cppnetlib 0.12 EXACT REQUIRED)
find_package(libtins REQUIRED)
find_package(TinyProcess REQUIRED)

pkg_check_modules(FLUID_BASE REQUIRED libfluid_base)
pkg_check_modules(FLUID_MSG REQUIRED libfluid_msg)
pkg_check_modules(EDITLINE REQUIRED libedit)

find_path(RANGE_V3_INCLUDE_DIRS
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
      ${RANGE_V3_INCLUDE_DIRS}
      ${FLUID_BASE_INCLUDE_DIRS}
      ${FLUID_MSG_INCLUDE_DIRS}
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR} # for Qt-generated moc files
      ${GLOG_INCLUDE_DIRS}
      ${LIBTINS_INCLUDE_DIRS}
    )

target_include_directories(runos_cli SYSTEM
    PRIVATE
      ${EDITLINE_INCLUDE_DIRS}
    )

target_include_directories(runos_rest SYSTEM
    PUBLIC
      ${CPPNETLIB_INCLUDE_DIRS}
    )

link_directories(
    ${EDITLINE_LIBRARY_DIRS}
    ${FLUID_BASE_LIBRARY_DIRS}
    ${FLUID_MSG_LIBRARY_DIRS}
    ${GLOG_LIBRARY_DIRS}
)

target_compile_definitions(runos
    PUBLIC ${BOOST_THREAD_CONFIG}
    )

target_link_libraries(runos
    PUBLIC
      Qt5::Core
      runos_core
    PRIVATE
      ${FLUID_BASE_LIBRARIES}
      ${FLUID_MSG_LIBRARIES}
      ${GLOG_LIBRARIES}
      ${LIBTINS_LIBRARIES}
      TinyProcess::TinyProcess
      heartbeatcore
      redisdb
      cpqd_print
    )

target_link_libraries(runos_cli
    PRIVATE
      runos
      ${EDITLINE_LIBRARIES}
    )

target_link_libraries(runos_rest
    PRIVATE
      runos
      ${CPPNETLIB_LIBRARIES}
    )

set_target_properties(runos PROPERTIES
    LANGUAGE CXX
    AUTOMOC ON
    )

add_subdirectory(hb)
add_subdirectory(redisdb)
add_subdirectory(oflog)
