add_subdirectory(cpqd-print)

target_sources(runos
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/OFLog.cc
    )
