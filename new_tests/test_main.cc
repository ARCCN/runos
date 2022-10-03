#include <gtest/gtest.h>

TEST(HelloTest, BasicAssertions) {
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    auto code = RUN_ALL_TESTS();
    std::cout << "I am going to SEGFAULT, please ignore it.." << std::endl;
    return code;
}
                                            
