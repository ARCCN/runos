add_executable(ethaddrTest ethaddrTest.cc)
target_link_libraries(ethaddrTest
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${TEST_LINK_LIBRARIES}
    runos_types)
add_test(NAME ethaddrTest COMMAND ethaddrTest)
