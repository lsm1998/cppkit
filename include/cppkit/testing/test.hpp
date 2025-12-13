#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace cppkit::testing
{
    class FatalAssertionException final : public std::exception
    {
    public:
        [[nodiscard]] const char* what() const noexcept override
        {
            return "Fatal assertion failed";
        }
    };

    class Test
    {
    public:
        virtual void SetUp()
        {
        }

        virtual void TearDown()
        {
        }

        virtual ~Test() = default;
    };

    class TestRegistry
    {
    public:
        using TestCreator = std::function<void()>;

        struct TestInfo
        {
            std::string suite_name;
            std::string test_name;
            TestCreator creator;
        };

        static TestRegistry& GetInstance()
        {
            static TestRegistry instance;
            return instance;
        }

        void RegisterTest(const std::string& suite_name, const std::string& test_name,
                          const TestCreator& creator)
        {
            tests_.push_back({suite_name, test_name, creator});
        }

        const std::vector<TestInfo>& GetTests() const { return tests_; }

    private:
        std::vector<TestInfo> tests_;
    };

    inline int RunAllTests()
    {
        int passed_count = 0;
        int failed_count = 0;
        int total_count = 0;

        std::cout << "[==========] Running all tests." << std::endl;

        for (const auto& [suite_name, test_name, creator] : TestRegistry::GetInstance().GetTests())
        {
            std::cout << "[ RUN      ] " << suite_name << "."
                << test_name << std::endl;
            total_count++;

            try
            {
                creator();
                std::cout << "[       OK ] " << suite_name << "."
                    << test_name << std::endl;
                passed_count++;
            }
            catch (const FatalAssertionException&)
            {
                // 致命断言已打印错误信息，直接标记为失败
                std::cout << "[  FAILED  ] " << suite_name << "."
                    << test_name << std::endl;
                failed_count++;
            }
            catch (const std::exception& e)
            {
                std::cerr << "意外异常: " << e.what() << std::endl;
                std::cout << "[  FAILED  ] " << suite_name << "."
                    << test_name << std::endl;
                failed_count++;
            }
            catch (...)
            {
                std::cerr << "意外异常" << std::endl;
                std::cout << "[  FAILED  ] " << suite_name << "."
                    << test_name << std::endl;
                failed_count++;
            }
        }

        std::cout << "[==========] " << total_count << " tests ran." << std::endl;
        std::cout << "[  PASSED  ] " << passed_count << " tests." << std::endl;
        if (failed_count > 0)
        {
            std::cout << "[  FAILED  ] " << failed_count << " tests." << std::endl;
        }

        return failed_count > 0 ? 1 : 0;
    }

    class TestResult
    {
    public:
        static bool& CurrentTestStatus()
        {
            thread_local bool status = true;
            return status;
        }

        static void PrintFailure(const std::string& expression,
                                 const std::string& file, int line)
        {
            std::cerr << file << ":" << line << ": failed" << std::endl;
            std::cerr << "  Expression: " << expression << std::endl;
        }
    };

#define EXPECT_TRUE(expr)                                                      \
  if (!(expr)) {                                                               \
    TestResult::CurrentTestStatus() = false;                                   \
    TestResult::PrintFailure(#expr, __FILE__, __LINE__);                       \
  }

#define EXPECT_EQ(expected, actual)                                            \
  if (!((expected) == (actual))) {                                             \
    TestResult::CurrentTestStatus() = false;                                   \
    TestResult::PrintFailure(#expected " == " #actual, __FILE__, __LINE__);    \
    std::cerr << "    expected value: " << (expected) << std::endl;            \
    std::cerr << "    actual value: " << (actual) << std::endl;                \
  }

#define ASSERT_TRUE(expr)                                                      \
  if (!(expr)) {                                                               \
    TestResult::PrintFailure(#expr, __FILE__, __LINE__);                       \
    throw FatalAssertionException();                                           \
  }

#define ASSERT_EQ(expected, actual)                                            \
  if (!((expected) == (actual))) {                                             \
    TestResult::PrintFailure(#expected " == " #actual, __FILE__, __LINE__);    \
    std::cerr << "    expected value: " << (expected) << std::endl;            \
    std::cerr << "    actual value: " << (actual) << std::endl;                \
    throw FatalAssertionException();                                           \
  }

#define TEST(suite, name)                                                      \
  namespace {                                                                  \
  class Test_##suite##_##name {                                                \
  public:                                                                      \
    static void RunTest() {                                                    \
      TestResult::CurrentTestStatus() = true;                                  \
      Body();                                                                  \
    }                                                                          \
                                                                               \
  private:                                                                     \
    static void Body();                                                        \
  };                                                                           \
                                                                               \
  [[maybe_unused]] bool Test_##suite##_##name##_registered = []() {            \
    TestRegistry::GetInstance().RegisterTest(#suite, #name,                    \
                                             Test_##suite##_##name::RunTest);  \
    return true;                                                               \
  }();                                                                         \
  }                                                                            \
  void Test_##suite##_##name::Body()

#define TEST_F(suite, name)                                                    \
  namespace {                                                                  \
  class Test_##suite##_##name##_ : public suite {                              \
  public:                                                                      \
    static void RunTest() {                                                    \
      Test_##suite##_##name##_ fixture;                                        \
      fixture.SetUp();                                                         \
      try {                                                                    \
        fixture.TestBody();                                                    \
      } catch (...) {                                                          \
        fixture.TearDown();                                                    \
        throw;                                                                 \
      }                                                                        \
      fixture.TearDown();                                                      \
    }                                                                          \
                                                                               \
  private:                                                                     \
    void TestBody();                                                           \
  };                                                                           \
                                                                               \
  [[maybe_unused]] bool Test_##suite##_##name##_registered = []() {            \
    TestRegistry::GetInstance().RegisterTest(                                  \
        #suite, #name, Test_##suite##_##name##_::RunTest);                     \
    return true;                                                               \
  }();                                                                         \
  }                                                                            \
  void Test_##suite##_##name##_::TestBody()
} // namespace cppkit
