#include "systemtrayicon/systemtrayicon.h"

#include "core/song.h"

#include <gtest/gtest.h>

TEST(SystemTrayIcon, NowPlayingUpdatesTooltip) {
  SystemTrayIcon tray;
  EXPECT_EQ("Strawberry", tray.tooltip());
  EXPECT_FALSE(tray.playing());
  Song song;
  song.set_title("Roads");
  song.set_artist("Portishead");
  song.set_valid(true);
  tray.SetNowPlaying(song);
  EXPECT_EQ("Portishead - Roads", tray.tooltip());
  tray.SetPlaying(true);
  tray.SetProgress(40);
  EXPECT_TRUE(tray.playing());
  EXPECT_EQ(40, tray.progress());
  tray.ClearNowPlaying();
  EXPECT_EQ("Strawberry", tray.tooltip());
}
