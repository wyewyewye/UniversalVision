#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <Poco/StringTokenizer.h>

using Poco::StringTokenizer;

TEST(BaseTest, BaseTest1)
{
    EXPECT_EQ(1, 1);
		std::string tokens = "white; black; magenta, blue, green; yellow";
		StringTokenizer tokenizer(tokens, ";,", StringTokenizer::TOK_TRIM);
		for (StringTokenizer::Iterator it = tokenizer.begin(); it != tokenizer.end(); ++it)
		{
			std::cout << *it << std::endl;
		}
}

int main()
{
    // Include testing::InitGoogleTest();
    testing::InitGoogleMock();
    return RUN_ALL_TESTS();
}