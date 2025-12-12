#include "cppkit/test.hpp"

using namespace cppkit;

int Add(const int a, const int b)
{
    return a + b;
}

TEST(MathSuite, CanAddTwoNumbers)
{
    EXPECT_EQ(5, Add(2, 3));
    EXPECT_EQ(0, Add(-1, 1));
}

TEST(MathSuite, AssertFailureStopsTest)
{
    ASSERT_EQ(10, Add(5, 5));
    ASSERT_EQ(20, Add(10, 9));
    EXPECT_TRUE(false);
}

class StringFixture : public Test
{
protected:
    void SetUp() override
    {
        // 在每个测试前运行
        str_ = "Hello, World";
        std::cout << "StringFixture SetUp" << std::endl;
    }

    void TearDown() override
    {
        // 在每个测试后运行
        str_.clear();
        std::cout << "StringFixture TearDown" << std::endl;
    }

    std::string str_;
};

TEST_F(StringFixture, SubstringTest)
{
    ASSERT_EQ("Hello", str_.substr(0, 5));
}

int main()
{
    return RunAllTests();
}
