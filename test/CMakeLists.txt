# Google Mock
#################################
set(GMOCK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/gmock-1.7.0)
add_subdirectory(${GMOCK_ROOT})

# Boost Test
#################################
add_definitions(
    -DBOOST_TEST_DYN_LINK
)

# Header files
##################################
include_directories(
    ${CMAKE_SOURCE_DIR}/test
    ${GMOCK_ROOT}/include
    ${GMOCK_ROOT}/gtest/include
)

# Link libraries
##################################
set(TEST_LINK_LIBRARIES
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    gmock
    )

# Subsystems
##################################
add_subdirectory(types)
add_subdirectory(oxm)
#add_subdirectory(maple)
