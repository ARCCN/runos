# Find dependencies
find_package(PkgConfig REQUIRED)
find_package(Boost 1.64 REQUIRED COMPONENTS thread filesystem)
set(BOOST_THREAD_CONFIG
    BOOST_THREAD_PROVIDES_FUTURE
    BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
    BOOST_THREAD_PROVIDES_SHARED_FUTURE_CONTINUATION
    BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
    BOOST_THREAD_PROVIDES_FUTURE_UNWRAP
    BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET
    BOOST_THREAD_PROVIDES_EXECUTORS)
find_package(Hana 1.0.0 REQUIRED)

if (NOT RUNOS_DISABLE_CRASH_REPORTING)
    find_package(Breakpad REQUIRED)
endif()

# TODO: To be replaced with spdlog
pkg_check_modules(GLOG REQUIRED libglog)
link_directories( ${GLOG_LIBRARY_DIRS} )

# Add sources
enable_language(ASM)
add_library(runos_core STATIC
    exception.cc
    assert.cc
    catch_all.cc
    demangle.cc
    crash_reporter.cc
    json11.cpp
)

if (NOT RUNOS_DISABLE_CRASH_REPORTING)
    target_sources(runos_core
        precatch_shim_personality.cc
        precatch_shim.x86-64.s
    )
endif()

# Configure target
# set_target_properties(runos_core PROPERTIES
#     LANGUAGE CXX
#     )

target_include_directories(runos_core
    PUBLIC
      ${CMAKE_SOURCE_DIR}/include
      ${Hana_INCLUDE_DIRS}
    PRIVATE
      ${GLOG_INCLUDE_DIRS}
    )

target_link_libraries(runos_core
    PUBLIC
      Boost::boost
      Boost::filesystem
      Boost::thread
      fmt
    PRIVATE
#      Breakpad::Breakpad
      ${GLOG_LIBRARIES}
    )

target_compile_definitions(runos_core
    PUBLIC ${BOOST_THREAD_CONFIG}
    )

##################
# Crash reporter #
##################
