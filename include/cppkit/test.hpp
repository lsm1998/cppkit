#pragma once

#include "test_core.hpp"

namespace cppkit {

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