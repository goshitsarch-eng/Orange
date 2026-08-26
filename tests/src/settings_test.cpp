#include "core/playerresume.h"
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

TEST(PlayerResume, ResumeAndPauseRules) {
  EXPECT_STREQ("Player", PlayerResume::kSettingsGroup);
  EXPECT_STREQ("playback_state", PlayerResume::kPlaybackState);
  EXPECT_FALSE(PlayerResume::ShouldResume(false, static_cast<int>(EngineBase::State::Playing)));
  EXPECT_TRUE(PlayerResume::ShouldResume(true, static_cast<int>(EngineBase::State::Playing)));
  EXPECT_TRUE(PlayerResume::ShouldResume(true, static_cast<int>(EngineBase::State::Paused)));
  EXPECT_FALSE(PlayerResume::ShouldResume(true, static_cast<int>(EngineBase::State::Idle)));
  EXPECT_TRUE(PlayerResume::ShouldPause(static_cast<int>(EngineBase::State::Paused)));
  EXPECT_FALSE(PlayerResume::ShouldPause(static_cast<int>(EngineBase::State::Playing)));
  EXPECT_EQ(5000000000LL, PlayerResume::PositionToNanosec(5));
  EXPECT_EQ(0, PlayerResume::PositionToNanosec(-3));
}
