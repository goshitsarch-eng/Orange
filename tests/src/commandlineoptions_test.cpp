#include "core/commandlineoptions.h"
#include "core/commandlinewindow.h"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

TEST(CommandlineOptions, SetUrlsAreApplied) {
  CommandlineOptions options;
  EXPECT_TRUE(options.urls().empty());
  options.set_urls({"file:///tmp/a.flac", "https://example.invalid/stream"});
  ASSERT_EQ(2u, options.urls().size());
  EXPECT_EQ("file:///tmp/a.flac", options.urls().front());
  EXPECT_TRUE(options.contains_play_options());
}

TEST(CommandlineWindow, ParsesResizeAndRaise) {
  int width = 0;
  int height = 0;
  EXPECT_TRUE(CommandlineWindow::ParseSize("1280x720", &width, &height));
  EXPECT_EQ(1280, width);
  EXPECT_EQ(720, height);
  EXPECT_FALSE(CommandlineWindow::ParseSize("nope", &width, &height));
  EXPECT_TRUE(CommandlineWindow::ShouldResize(CommandlineOptions::PlayerAction::ResizeWindow, 800, 600));
  EXPECT_FALSE(CommandlineWindow::ShouldResize(CommandlineOptions::PlayerAction::Play, 800, 600));
  CommandlineOptions empty;
  EXPECT_TRUE(CommandlineWindow::ShouldRaise(empty));
}

TEST(CommandlineOptions, ParsesResizeVersionAndLogLevels) {
  auto parse = [](std::vector<const char *> args) {
    args.insert(args.begin(), "strawberry");
    std::vector<char *> argv;
    for (const char *arg : args) {
      argv.push_back(const_cast<char *>(arg));
    }
    CommandlineOptions options;
    EXPECT_TRUE(options.Parse(static_cast<int>(argv.size()), argv.data()));
    return options;
  };
  const CommandlineOptions resize = parse({"--resize-window", "1024x768"});
  EXPECT_EQ(CommandlineOptions::PlayerAction::ResizeWindow, resize.player_action());
  EXPECT_EQ(1024, resize.resize_width());
  EXPECT_EQ(768, resize.resize_height());
  const CommandlineOptions version = parse({"--version"});
  EXPECT_TRUE(version.version());
  const CommandlineOptions verbose = parse({"--verbose"});
  EXPECT_EQ("*:4", verbose.log_levels());
  EXPECT_TRUE(verbose.debug());
  const CommandlineOptions quiet = parse({"--quiet"});
  EXPECT_EQ("*:1", quiet.log_levels());
}
