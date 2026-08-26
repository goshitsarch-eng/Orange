#include "utilities/strutils.h"
#include "utilities/timeutils.h"
#include "utilities/fileutils.h"

#include <gtest/gtest.h>

TEST(TimeUtils, PrettyTime) {
  EXPECT_EQ("0:00", Utilities::PrettyTime(0));
  EXPECT_EQ("1:01", Utilities::PrettyTime(61));
  EXPECT_EQ("1:01:01", Utilities::PrettyTime(3661));
}

TEST(TimeUtils, PrettyTimeNanosec) {
  EXPECT_EQ("0:05", Utilities::PrettyTimeNanosec(5000000000LL));
}

TEST(StrUtils, SplitJoin) {
  const auto parts = StrUtils::Split("a,b,c", ',');
  ASSERT_EQ(3u, parts.size());
  EXPECT_EQ("a,b,c", StrUtils::Join(parts, ","));
}

TEST(StrUtils, CaseAndTrim) {
  EXPECT_EQ("hello", StrUtils::ToLower("HeLLo"));
  EXPECT_EQ("hello", StrUtils::Trim("  hello\n"));
  EXPECT_TRUE(StrUtils::StartsWith("strawberry", "straw"));
  EXPECT_TRUE(StrUtils::ContainsInsensitive("Album Artist", "artist"));
}

TEST(FileUtils, BaseAndExtension) {
  EXPECT_EQ("song.mp3", FileUtils::BaseName("/tmp/music/song.mp3"));
  EXPECT_EQ("mp3", FileUtils::Extension("/tmp/music/song.mp3"));
}
