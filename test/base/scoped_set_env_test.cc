//

#ifdef _WIN32
#include <windows.h>
#endif

#include <gtest/gtest.h>
#include <abel/base/internal/scoped_set_env.h>

namespace {

using abel::base_internal::ScopedSetEnv;

std::string GetEnvVar(const char* name) {
#ifdef _WIN32
  char buf[1024];
  auto get_res = GetEnvironmentVariableA(name, buf, sizeof(buf));
  if (get_res >= sizeof(buf)) {
    return "TOO_BIG";
  }

  if (get_res == 0) {
    return "UNSET";
  }

  return std::string(buf, get_res);
#else
  const char* val = ::getenv(name);
  if (val == nullptr) {
    return "UNSET";
  }

  return val;
#endif
}

TEST(ScopedSetEnvTest, SetNonExistingVarToString) {
  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");

  {
    ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", "value");

    EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");
  }

  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
}

TEST(ScopedSetEnvTest, SetNonExistingVarToNull) {
  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");

  {
    ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", nullptr);

    EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
  }

  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
}

TEST(ScopedSetEnvTest, SetExistingVarToString) {
  ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", "value");
  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");

  {
    ScopedSetEnv scoped_set_env("SCOPED_SET_ENV_TEST_VAR", "new_value");

    EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "new_value");
  }

  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");
}

TEST(ScopedSetEnvTest, SetExistingVarToNull) {
  ScopedSetEnv scoped_set("SCOPED_SET_ENV_TEST_VAR", "value");
  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");

  {
    ScopedSetEnv scoped_set_env("SCOPED_SET_ENV_TEST_VAR", nullptr);

    EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "UNSET");
  }

  EXPECT_EQ(GetEnvVar("SCOPED_SET_ENV_TEST_VAR"), "value");
}

}  // namespace
