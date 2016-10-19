#pragma once

#include <boost/test/unit_test.hpp>
#include <gmock/gmock.h>

struct InitGMock {
    InitGMock() {
        ::testing::GTEST_FLAG(throw_on_failure) = true;
        ::testing::InitGoogleMock(&boost::unit_test::framework::master_test_suite().argc,
                                   boost::unit_test::framework::master_test_suite().argv);
    }
};
BOOST_GLOBAL_FIXTURE(InitGMock);

