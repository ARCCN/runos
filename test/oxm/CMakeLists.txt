#add_executable(queryTest queryTest.cc)
#target_link_libraries(queryTest
#    ${TEST_LINK_LIBRARIES}
#    )
#add_test(NAME queryTest COMMAND queryTest)
#
add_executable(fieldTest fieldTest.cc)
target_link_libraries(fieldTest
    ${TEST_LINK_LIBRARIES}
    runos_types
    )
add_test(NAME fieldTest COMMAND fieldTest)
