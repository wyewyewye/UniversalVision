#include "gtest/gtest.h"
#include "gmock/gmock.h"


TEST(BaseTest, BaseTest1)
{
    EXPECT_EQ(1, 1);
}

int main()
{
    // Include testing::InitGoogleTest();
    testing::InitGoogleMock();
    return RUN_ALL_TESTS();
}