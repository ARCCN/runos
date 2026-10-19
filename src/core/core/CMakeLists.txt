# Find dependencies
find_package(spdlog REQUIRED)
find_package(Boost 1.69 REQUIRED COMPONENTS thread filesystem)

if (NOT RUNOS_DISABLE_CRASH_REPORTING)
    find_package(Breakpad REQUIRED)
endif()

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
 set_target_properties(runos_core PROPERTIES
     LANGUAGE CXX
     )

target_include_directories(runos_core
    PUBLIC
      ${CMAKE_SOURCE_DIR}/include
      ${spdlog_INCLUDE_DIRS}
      ${Boost_INCLUDE_DIRS}
    PRIVATE
    )

target_link_libraries(runos_core
    PUBLIC
      Boost::boost
      Boost::filesystem
      Boost::thread
      ${spdlog_LIBRARIES}
    )


##################
# Crash reporter #
##################
