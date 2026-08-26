#include "systemtrayicon/systemtrayicon.h"

#include "constants/behavioursettings.h"
#include "core/settings.h"
#include "core/song.h"
#include "systemtrayicon/trayprogressoverlay.h"

#include <algorithm>

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
  EXPECT_STREQ("Play", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuPlayPause, false));
  EXPECT_STREQ("Pause", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuPlayPause, true));
  EXPECT_STREQ("Stop", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuStop, false));
  EXPECT_STREQ("Next", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuNext, false));
  EXPECT_STREQ("Previous", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuPrevious, false));
  EXPECT_STREQ("Mute", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuMute, false));
  EXPECT_STREQ("Stop after this track", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuStopAfter, false));
  EXPECT_STREQ("Love", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuLove, false));
  EXPECT_STREQ("Show / Hide", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuShowHide, false));
  EXPECT_STREQ("Quit", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuQuit, false));
  EXPECT_STREQ("", SystemTrayIcon::MenuLabel(SystemTrayIcon::kMenuSeparator, false));
  EXPECT_TRUE(SystemTrayIcon::IsSeparatorId(SystemTrayIcon::kMenuSeparator));
  EXPECT_FALSE(SystemTrayIcon::IsSeparatorId(SystemTrayIcon::kMenuMute));
  const auto ids = SystemTrayIcon::RootMenuIds();
  EXPECT_EQ(10u, ids.size());
  EXPECT_NE(ids.end(), std::find(ids.begin(), ids.end(), SystemTrayIcon::kMenuMute));
  EXPECT_NE(ids.end(), std::find(ids.begin(), ids.end(), SystemTrayIcon::kMenuStopAfter));
  EXPECT_NE(ids.end(), std::find(ids.begin(), ids.end(), SystemTrayIcon::kMenuLove));

  SystemTrayIcon tray;
  int play = 0;
  int quit = 0;
  int mute = 0;
  int stop_after = 0;
  int love = 0;
  tray.PlayPause.Connect([&]() { ++play; });
  tray.Quit.Connect([&]() { ++quit; });
  tray.Mute.Connect([&]() { ++mute; });
  tray.StopAfter.Connect([&]() { ++stop_after; });
  tray.Love.Connect([&]() { ++love; });
  EXPECT_TRUE(SystemTrayIcon::ActivateMenuId(SystemTrayIcon::kMenuPlayPause, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous,
                                            &tray.ShowHide, &tray.Quit, &tray.Mute, &tray.StopAfter, &tray.Love));
  EXPECT_TRUE(SystemTrayIcon::ActivateMenuId(SystemTrayIcon::kMenuQuit, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous,
                                            &tray.ShowHide, &tray.Quit, &tray.Mute, &tray.StopAfter, &tray.Love));
  EXPECT_TRUE(SystemTrayIcon::ActivateMenuId(SystemTrayIcon::kMenuMute, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous,
                                            &tray.ShowHide, &tray.Quit, &tray.Mute, &tray.StopAfter, &tray.Love));
  EXPECT_TRUE(SystemTrayIcon::ActivateMenuId(SystemTrayIcon::kMenuStopAfter, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous,
                                            &tray.ShowHide, &tray.Quit, &tray.Mute, &tray.StopAfter, &tray.Love));
  EXPECT_TRUE(SystemTrayIcon::ActivateMenuId(SystemTrayIcon::kMenuLove, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous,
                                            &tray.ShowHide, &tray.Quit, &tray.Mute, &tray.StopAfter, &tray.Love));
  EXPECT_FALSE(SystemTrayIcon::ActivateMenuId(SystemTrayIcon::kMenuSeparator, &tray.PlayPause, &tray.Stop, &tray.Next, &tray.Previous,
                                             &tray.ShowHide, &tray.Quit, &tray.Mute, &tray.StopAfter, &tray.Love));
  EXPECT_EQ(1, play);
  EXPECT_EQ(1, quit);
  EXPECT_EQ(1, mute);
  EXPECT_EQ(1, stop_after);
  EXPECT_EQ(1, love);
  EXPECT_EQ("/NO_DBUSMENU", tray.menu_path());
  EXPECT_EQ(1u, tray.menu_revision());
}

TEST(TrayProgressOverlay, DecadeAndIconName) {
  EXPECT_EQ(0, TrayProgressOverlay::Decade(-10));
  EXPECT_EQ(0, TrayProgressOverlay::Decade(9));
  EXPECT_EQ(40, TrayProgressOverlay::Decade(44));
  EXPECT_EQ(100, TrayProgressOverlay::Decade(100));
  EXPECT_EQ(100, TrayProgressOverlay::Decade(140));
  EXPECT_TRUE(TrayProgressOverlay::IconName(40, false, true).empty());
  EXPECT_TRUE(TrayProgressOverlay::IconName(40, true, false).empty());
  EXPECT_TRUE(TrayProgressOverlay::IconName(0, true, true).empty());
  EXPECT_EQ("strawberry-progress-40", TrayProgressOverlay::IconName(44, true, true));
}

TEST(SystemTrayIcon, OverlayIconNameHonorsProgressSetting) {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  settings.SetBoolValue(BehaviourSettings::kTrayIconProgress, true);
  settings.Sync();

  SystemTrayIcon tray;
  tray.SetPlaying(true);
  tray.SetProgress(44);
  EXPECT_EQ("strawberry-progress-40", tray.OverlayIconName());

  settings.SetBoolValue(BehaviourSettings::kTrayIconProgress, false);
  settings.Sync();
  EXPECT_TRUE(tray.OverlayIconName().empty());
}
