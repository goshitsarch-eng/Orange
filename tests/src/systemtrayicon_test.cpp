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

TEST(SystemTrayIcon, DBusMenuLabelsAndActions) {
  EXPECT_STREQ("Play", SystemTrayIcon::MenuLabel(1, false));
  EXPECT_STREQ("Pause", SystemTrayIcon::MenuLabel(1, true));
  EXPECT_STREQ("Stop", SystemTrayIcon::MenuLabel(2, false));
  EXPECT_STREQ("Next", SystemTrayIcon::MenuLabel(3, false));
  EXPECT_STREQ("Previous", SystemTrayIcon::MenuLabel(4, false));
  EXPECT_STREQ("Show / Hide", SystemTrayIcon::MenuLabel(6, false));
  EXPECT_STREQ("Quit", SystemTrayIcon::MenuLabel(7, false));
  EXPECT_STREQ("", SystemTrayIcon::MenuLabel(5, false));

  SystemTrayIcon tray;
  int play = 0;
  int quit = 0;
  tray.PlayPause.Connect([&]() { ++play; });
  tray.Quit.Connect([&]() { ++quit; });
  EXPECT_TRUE(SystemTrayIcon::ActivateMenuId(1, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous, &tray.ShowHide, &tray.Quit));
  EXPECT_TRUE(SystemTrayIcon::ActivateMenuId(7, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous, &tray.ShowHide, &tray.Quit));
  EXPECT_FALSE(SystemTrayIcon::ActivateMenuId(5, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous, &tray.ShowHide, &tray.Quit));
  EXPECT_EQ(1, play);
  EXPECT_EQ(1, quit);
  EXPECT_EQ("/NO_DBUSMENU", tray.menu_path());
  EXPECT_EQ(1u, tray.menu_revision());
}
