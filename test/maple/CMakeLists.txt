add_executable(TraceablePacketTest TraceablePacketTest.cc)
target_link_libraries(TraceablePacketTest
    ${TEST_LINK_LIBRARIES}
    runos_types
    runos_maple
    )
add_test(NAME TraceablePacketTest COMMAND TraceablePacketTest)

add_executable(TraceTreeTest TraceTreeTest.cc)
target_link_libraries(TraceTreeTest
    ${TEST_LINK_LIBRARIES}
    runos_types
    runos_maple
    )
add_test(NAME TraceTreeTest COMMAND TraceTreeTest)
