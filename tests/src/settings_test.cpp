#include "core/settings.h"
#include "core/standardpaths.h"

#include <gtest/gtest.h>

TEST(Settings, RoundTrip) {
  Settings settings;
  settings.BeginGroup("Behaviour");
  settings.SetBoolValue("test_flag", true);
  settings.SetIntValue("test_int", 42);
  settings.SetValue("test_str", "strawberry");
  EXPECT_TRUE(settings.BoolValue("test_flag"));
  EXPECT_EQ(42, settings.IntValue("test_int"));
  EXPECT_EQ("strawberry", settings.Value("test_str"));
}
