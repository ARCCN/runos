file(GLOB SRC *.cc)

add_executable(runos_test
  ${SRC}
)

target_link_libraries(runos_test
  PUBLIC
    runos
    gtest
  )

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# following lines can be used to run tests after build
# unfortunately tests fail due to usage of static memory and invalid destructors
# in runos library causing build failure and removal of test binary

include(GoogleTest)
gtest_discover_tests(runos_test)
