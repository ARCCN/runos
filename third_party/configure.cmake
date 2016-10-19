set(FLUID_BASE_FILES ${CMAKE_BINARY_DIR}/libfluid_base_files)
if (NOT EXISTS ${FLUID_BASE_FILES}/config.status)
    message("Configuring libfluid_base...")

    file(MAKE_DIRECTORY ${FLUID_BASE_FILES})
    execute_process(
        COMMAND ${CMAKE_SOURCE_DIR}/third_party/libfluid_base/configure
            --prefix=${CMAKE_BINARY_DIR}/prefix
        WORKING_DIRECTORY ${FLUID_BASE_FILES}
        RESULT_VARIABLE FLUID_BASE_CONFIGURED
    )
    
    if (NOT ${FLUID_BASE_CONFIGURED} EQUAL 0)
        message( FATAL_ERROR "Can't configure libfluid_base" )
    endif()
endif()

set(FLUID_MSG_FILES ${CMAKE_BINARY_DIR}/libfluid_msg_files)
if (NOT EXISTS ${FLUID_MSG_FILES}/config.status)
    message("Configuring libfuild_msg...")

    file(MAKE_DIRECTORY ${FLUID_MSG_FILES})
    execute_process(
        COMMAND ${CMAKE_SOURCE_DIR}/third_party/libfluid_msg/configure
            --prefix=${CMAKE_BINARY_DIR}/prefix
            CXXFLAGS=-DIFHWADDRLEN=6 # OS X compilation fix
        WORKING_DIRECTORY ${FLUID_MSG_FILES}
        RESULT_VARIABLE FLUID_MSG_CONFIGURED
    )
    
    if (NOT ${FLUID_MSG_CONFIGURED} EQUAL 0)
        message( FATAL_ERROR "Can't configure libfluid_msg" )
    endif()
endif()

# Build third-party libraries
add_custom_target(fluid_base_install
    COMMAND make install
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/libfluid_base_files
    COMMENT "Building libfluid_base"
)

add_custom_target(fluid_msg_install
    COMMAND make install
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/libfluid_msg_files
    COMMENT "Building libfluid_msg"
)

add_custom_target(prefix)
add_dependencies(prefix
    fluid_base_install
    fluid_msg_install
)
