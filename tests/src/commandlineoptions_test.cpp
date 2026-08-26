#include "core/commandlineoptions.h"

#include <gtest/gtest.h>

TEST(CommandlineOptions, SetUrlsAreApplied) {
  CommandlineOptions options;
  EXPECT_TRUE(options.urls().empty());
  options.set_urls({"file:///tmp/a.flac", "https://example.invalid/stream"});
  ASSERT_EQ(2u, options.urls().size());
  EXPECT_EQ("file:///tmp/a.flac", options.urls().front());
  EXPECT_TRUE(options.contains_play_options());
}
