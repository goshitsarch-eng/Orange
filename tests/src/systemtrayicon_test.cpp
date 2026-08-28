#include "core/macstartupactions.h"
#include "ui/mainwindowlook.h"
#include "core/playbackcontrolsstate.h"
#include "core/playeritemoptions.h"
#include "systemtrayicon/systemtrayicon.h"
#include "systemtrayicon/traymenulove.h"
#include "systemtrayicon/traymenumute.h"
#include "systemtrayicon/traymenuplaypause.h"
#include "systemtrayicon/traymenustop.h"
#include "widgets/busyindicatoranim.h"

#include "constants/behavioursettings.h"
#include "core/settings.h"
#include "core/song.h"
#include "systemtrayicon/trayiconcomposite.h"
#include "systemtrayicon/trayiconmask.h"
#include "systemtrayicon/trayiconpixmap.h"
#include "systemtrayicon/traymenuposition.h"
#include "systemtrayicon/traypopup.h"
#include "systemtrayicon/trayprogressoverlay.h"
#include "systemtrayicon/traysettingsreload.h"

#include <algorithm>

#include <gtest/gtest.h>

TEST(SystemTrayIcon, NowPlayingUpdatesTooltip) {
  SystemTrayIcon tray;
  EXPECT_EQ("Orange", tray.tooltip());
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
  EXPECT_EQ("Orange", tray.tooltip());
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
  const auto hidden = SystemTrayIcon::RootMenuIds(false);
  EXPECT_EQ(9u, hidden.size());
  EXPECT_EQ(hidden.end(), std::find(hidden.begin(), hidden.end(), SystemTrayIcon::kMenuLove));
  EXPECT_EQ(ids, TrayMenuLove::FilterMenuIds(SystemTrayIcon::AllMenuIds(), SystemTrayIcon::kMenuLove, true));
  EXPECT_FALSE(TrayMenuLove::LoveEnabled(false, true, false));
  EXPECT_FALSE(TrayMenuLove::LoveEnabled(true, true, true));
  EXPECT_TRUE(TrayMenuLove::LoveEnabled(true, true, false));
  EXPECT_FALSE(TrayMenuLove::ItemVisible(SystemTrayIcon::kMenuLove, SystemTrayIcon::kMenuLove, false));
  EXPECT_TRUE(TrayMenuLove::ItemEnabled(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuLove, false));

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

TEST(TrayIconComposite, QtCoverGeometryAndOverlayNames) {
  EXPECT_STREQ("orange", TrayIconComposite::BaseIconName());
  EXPECT_EQ(TrayIconComposite::Playback::Stopped, TrayIconComposite::StateFrom(false, false));
  EXPECT_EQ(TrayIconComposite::Playback::Playing, TrayIconComposite::StateFrom(true, false));
  EXPECT_EQ(TrayIconComposite::Playback::Paused, TrayIconComposite::StateFrom(false, true));
  EXPECT_STREQ("media-playback-start", TrayIconComposite::BadgeIconName(TrayIconComposite::Playback::Playing));
  EXPECT_STREQ("media-playback-pause", TrayIconComposite::BadgeIconName(TrayIconComposite::Playback::Paused));
  EXPECT_STREQ("", TrayIconComposite::BadgeIconName(TrayIconComposite::Playback::Stopped));
  EXPECT_NEAR(TrayIconComposite::kHalfPi, TrayIconComposite::CoverAngle(0), 1e-9);
  EXPECT_NEAR(0.0, TrayIconComposite::CoverAngle(100), 1e-9);
  EXPECT_NEAR(TrayIconComposite::kHalfPi / 2.0, TrayIconComposite::CoverAngle(50), 1e-9);
  EXPECT_FALSE(TrayIconComposite::CoverIncludesBottomRight(50));
  EXPECT_TRUE(TrayIconComposite::CoverIncludesBottomRight(51));
  const TrayIconComposite::Point end = TrayIconComposite::CoverLineEnd(32, 32, 0);
  EXPECT_GT(end.x, end.y);
  EXPECT_EQ(16, TrayIconComposite::BadgeHeight(32));
  EXPECT_EQ(16, TrayIconComposite::BadgeTopLeft(32, 16).x);
  EXPECT_EQ(0, TrayIconComposite::BadgeTopLeft(32, 16).y);
  EXPECT_TRUE(TrayIconComposite::ShowsProgress(40, true, TrayIconComposite::Playback::Playing));
  EXPECT_TRUE(TrayIconComposite::ShowsProgress(40, true, TrayIconComposite::Playback::Paused));
  EXPECT_FALSE(TrayIconComposite::ShowsProgress(40, true, TrayIconComposite::Playback::Stopped));
  EXPECT_EQ("strawberry-progress-40", TrayIconComposite::OverlayName(44, true, TrayIconComposite::Playback::Playing));
  EXPECT_EQ("strawberry-progress-40", TrayIconComposite::OverlayName(44, true, TrayIconComposite::Playback::Paused));
  EXPECT_EQ("media-playback-pause", TrayIconComposite::OverlayName(0, true, TrayIconComposite::Playback::Paused));
  EXPECT_EQ("media-playback-start", TrayIconComposite::OverlayName(0, false, TrayIconComposite::Playback::Playing));
  EXPECT_TRUE(TrayIconComposite::OverlayName(40, true, TrayIconComposite::Playback::Stopped).empty());
}

TEST(TrayIconMask, ProgressPolygonMatchesQtWedge) {
  const auto early = TrayIconMask::ProgressMask(48, 48, 25);
  ASSERT_EQ(4u, early.size());
  EXPECT_EQ(0, early.front().x);
  EXPECT_EQ(0, early.front().y);
  EXPECT_EQ(0, early.back().x);
  EXPECT_EQ(48, early[early.size() - 2].x);
  const auto late = TrayIconMask::ProgressMask(48, 48, 80);
  ASSERT_EQ(5u, late.size());
  EXPECT_EQ(48, late[2].x);
  EXPECT_EQ(48, late[2].y);
}

TEST(TrayIconPixmap, ByteCountAndNetworkArgb) {
  EXPECT_EQ(48, TrayIconPixmap::kDefaultSize);
  EXPECT_TRUE(TrayIconPixmap::ValidDimensions(48, 48));
  EXPECT_FALSE(TrayIconPixmap::ValidDimensions(0, 48));
  EXPECT_FALSE(TrayIconPixmap::ValidDimensions(300, 48));
  EXPECT_EQ(9216u, TrayIconPixmap::ByteCount(48, 48));
  uint8_t pixel[4] = {};
  TrayIconPixmap::WriteNetworkArgb(pixel, 255, 12, 34, 56);
  EXPECT_EQ(255, pixel[0]);
  EXPECT_EQ(12, pixel[1]);
  EXPECT_EQ(34, pixel[2]);
  EXPECT_EQ(56, pixel[3]);
  std::vector<uint8_t> packed;
  const uint32_t row[] = {TrayIconPixmap::NativeArgb(255, 1, 2, 3)};
  TrayIconPixmap::PackNativeArgbRow(row, 1, &packed);
  ASSERT_EQ(4u, packed.size());
  EXPECT_EQ(255, packed[0]);
  EXPECT_EQ(1, packed[1]);
}

TEST(TrayMenuPosition, AnchorAndClamp) {
  const auto anchor = TrayMenuPosition::AnchorPoint(100, 200);
  EXPECT_EQ(100, anchor.x);
  EXPECT_EQ(200, anchor.y);
  EXPECT_EQ(1, anchor.width);
  EXPECT_FALSE(TrayMenuPosition::HasScreenPoint(0, 0));
  EXPECT_TRUE(TrayMenuPosition::HasScreenPoint(8, 0));
  const TrayMenuPosition::Rect monitor{0, 0, 1920, 1080};
  const auto clamped = TrayMenuPosition::ClampToMonitor({1900, 1070, 40, 40}, monitor);
  EXPECT_EQ(1880, clamped.x);
  EXPECT_EQ(1040, clamped.y);
}

TEST(TrayPopup, ArtAndFade) {
  EXPECT_TRUE(TrayPopup::ShowArt(true, true));
  EXPECT_FALSE(TrayPopup::ShowArt(false, true));
  EXPECT_FALSE(TrayPopup::ShowArt(true, false));
  EXPECT_DOUBLE_EQ(0.0, TrayPopup::FadeOpacity(0, 300, true));
  EXPECT_DOUBLE_EQ(1.0, TrayPopup::FadeOpacity(300, 300, true));
  EXPECT_EQ(300, TrayPopup::FadeDurationMs());
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
  EXPECT_EQ("media-playback-start", tray.OverlayIconName());

  tray.SetPaused();
  EXPECT_TRUE(tray.paused());
  EXPECT_FALSE(tray.playing());
  EXPECT_EQ("media-playback-pause", tray.OverlayIconName());
  tray.SetStopped();
  EXPECT_FALSE(tray.paused());
  EXPECT_FALSE(tray.playing());
  EXPECT_TRUE(tray.OverlayIconName().empty());
  EXPECT_EQ(0, tray.icon_pixmap_width());
  EXPECT_EQ(0u, tray.icon_pixmap_size());
  tray.ShowMenu(120, 80);
  EXPECT_EQ(120, tray.last_menu_x());
  EXPECT_EQ(80, tray.last_menu_y());
  tray.ShowPopup("title", "body", 1000, {1, 2, 3});
  ASSERT_EQ(3u, tray.popup_art().size());
  EXPECT_EQ(1, tray.popup_art()[0]);
}

TEST(MacStartupActions, ReopenDockAndMediaKeysMatchQt) {
  EXPECT_TRUE(MacStartupActions::ShouldHandleReopen());
  EXPECT_TRUE(MacStartupActions::ReopenReturnYes());
  EXPECT_TRUE(MacStartupActions::ShouldTerminateNow());
  EXPECT_EQ(8, MacStartupActions::AuxControlSubtype());
  EXPECT_TRUE(MacStartupActions::IsAuxControlSubtype(8));
  EXPECT_FALSE(MacStartupActions::IsAuxControlSubtype(0));
  EXPECT_EQ(16, MacStartupActions::MediaKeyCode((16 << 16) | (0x0B << 8)));
  EXPECT_EQ(0x0B00, MacStartupActions::MediaKeyFlags((16 << 16) | (0x0B << 8)));
  EXPECT_TRUE(MacStartupActions::IsMediaKeyReleased(0x0B00));
  EXPECT_FALSE(MacStartupActions::IsMediaKeyReleased(0x0A00));
  EXPECT_TRUE(MacStartupActions::ShouldHandleMediaEvent(8, 0x0B00));
  EXPECT_FALSE(MacStartupActions::ShouldHandleMediaEvent(8, 0x0A00));
  EXPECT_STREQ("Now Playing", MacStartupActions::NowPlayingLabel());
  const auto ids = MacStartupActions::DockMenuIds(true);
  EXPECT_EQ(SystemTrayIcon::RootMenuIds(true), ids);
  EXPECT_TRUE(MacStartupActions::DockItemIsSeparator(SystemTrayIcon::kMenuSeparator));
}

TEST(TrayMenuMute, CheckedStateMatchesQt) {
  EXPECT_TRUE(TrayMenuMute::IsToggleId(SystemTrayIcon::kMenuMute, SystemTrayIcon::kMenuMute));
  EXPECT_FALSE(TrayMenuMute::IsToggleId(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuMute));
  EXPECT_STREQ("checkmark", TrayMenuMute::ToggleType());
  EXPECT_EQ(0, TrayMenuMute::ToggleState(false));
  EXPECT_EQ(1, TrayMenuMute::ToggleState(true));
  EXPECT_EQ(-2, TrayMenuMute::ToggleStateForId(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuMute, true));
  EXPECT_EQ(1, TrayMenuMute::ToggleStateForId(SystemTrayIcon::kMenuMute, SystemTrayIcon::kMenuMute, true));
  EXPECT_TRUE(TrayMenuMute::ItemChecked(SystemTrayIcon::kMenuMute, SystemTrayIcon::kMenuMute, true));
  EXPECT_FALSE(TrayMenuMute::ItemChecked(SystemTrayIcon::kMenuMute, SystemTrayIcon::kMenuMute, false));
  EXPECT_FALSE(TrayMenuMute::ItemChecked(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuMute, true));
  EXPECT_TRUE(MainWindowLook::IsMuted(0));
  EXPECT_FALSE(MainWindowLook::IsMuted(1));

  SystemTrayIcon tray;
  EXPECT_FALSE(tray.mute_checked());
  tray.SetMuteChecked(true);
  EXPECT_TRUE(tray.mute_checked());
  tray.SetMuteChecked(false);
  EXPECT_FALSE(tray.mute_checked());
}

TEST(TrayMenuPlayPause, DisabledForRadioLikeQt) {
  EXPECT_TRUE(PlaybackControlsState::PlayPauseEnabled(false, true));
  EXPECT_TRUE(PlaybackControlsState::PlayPauseEnabled(false, false));
  EXPECT_TRUE(PlaybackControlsState::PlayPauseEnabled(true, false));
  EXPECT_FALSE(PlaybackControlsState::PlayPauseEnabled(true, true));
  Song radio;
  radio.set_source(Song::Source::Stream);
  EXPECT_TRUE(PlayerItemOptions::PauseDisabled(radio));
  EXPECT_FALSE(PlaybackControlsState::PlayPauseEnabled(true, PlayerItemOptions::PauseDisabled(radio)));
  EXPECT_TRUE(TrayMenuPlayPause::ItemEnabled(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuPlayPause, false));
  EXPECT_FALSE(TrayMenuPlayPause::ItemEnabled(SystemTrayIcon::kMenuPlayPause, SystemTrayIcon::kMenuPlayPause, false));
  EXPECT_TRUE(TrayMenuPlayPause::ItemEnabled(SystemTrayIcon::kMenuPlayPause, SystemTrayIcon::kMenuPlayPause, true));

  SystemTrayIcon tray;
  EXPECT_TRUE(tray.play_pause_enabled());
  tray.SetPlaying(true, false);
  EXPECT_TRUE(tray.playing());
  EXPECT_FALSE(tray.play_pause_enabled());
  tray.SetPaused();
  EXPECT_TRUE(tray.play_pause_enabled());
  tray.SetPlaying(true, true);
  EXPECT_TRUE(tray.play_pause_enabled());
  tray.SetStopped();
  EXPECT_TRUE(tray.play_pause_enabled());
}

TEST(TrayMenuStop, DisabledWhenStoppedLikeQt) {
  EXPECT_TRUE(TrayMenuStop::PlaybackActive(true, false));
  EXPECT_TRUE(TrayMenuStop::PlaybackActive(false, true));
  EXPECT_FALSE(TrayMenuStop::PlaybackActive(false, false));
  EXPECT_TRUE(TrayMenuStop::IsStopId(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStopAfter));
  EXPECT_TRUE(TrayMenuStop::IsStopId(SystemTrayIcon::kMenuStopAfter, SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStopAfter));
  EXPECT_FALSE(TrayMenuStop::IsStopId(SystemTrayIcon::kMenuNext, SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStopAfter));
  EXPECT_FALSE(TrayMenuStop::ItemEnabled(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStopAfter, false));
  EXPECT_FALSE(TrayMenuStop::ItemEnabled(SystemTrayIcon::kMenuStopAfter, SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStopAfter, false));
  EXPECT_TRUE(TrayMenuStop::ItemEnabled(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStopAfter, true));
  EXPECT_TRUE(TrayMenuStop::ItemEnabled(SystemTrayIcon::kMenuNext, SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuStopAfter, false));

  SystemTrayIcon tray;
  EXPECT_FALSE(TrayMenuStop::PlaybackActive(tray.playing(), tray.paused()));
  tray.SetPlaying(true);
  EXPECT_TRUE(TrayMenuStop::PlaybackActive(tray.playing(), tray.paused()));
  tray.SetPaused();
  EXPECT_TRUE(TrayMenuStop::PlaybackActive(tray.playing(), tray.paused()));
  tray.SetStopped();
  EXPECT_FALSE(TrayMenuStop::PlaybackActive(tray.playing(), tray.paused()));
}

TEST(TrayMenuMute, FollowsVolumeControlLikeQt) {
  EXPECT_TRUE(TrayMenuMute::ShouldShow(true));
  EXPECT_FALSE(TrayMenuMute::ShouldShow(false));
  EXPECT_TRUE(MainWindowLook::MuteVisible(true));
  EXPECT_FALSE(MainWindowLook::MuteVisible(false));
  const auto all = SystemTrayIcon::RootMenuIds();
  EXPECT_NE(all.end(), std::find(all.begin(), all.end(), SystemTrayIcon::kMenuMute));
  const auto hidden = SystemTrayIcon::RootMenuIds(true, false);
  EXPECT_EQ(9u, hidden.size());
  EXPECT_EQ(hidden.end(), std::find(hidden.begin(), hidden.end(), SystemTrayIcon::kMenuMute));
  EXPECT_NE(hidden.end(), std::find(hidden.begin(), hidden.end(), SystemTrayIcon::kMenuLove));
  EXPECT_EQ(hidden, TrayMenuMute::FilterMenuIds(SystemTrayIcon::AllMenuIds(), SystemTrayIcon::kMenuMute, false));
  EXPECT_FALSE(TrayMenuMute::ItemVisible(SystemTrayIcon::kMenuMute, SystemTrayIcon::kMenuMute, false));
  EXPECT_TRUE(TrayMenuMute::ItemVisible(SystemTrayIcon::kMenuStop, SystemTrayIcon::kMenuMute, false));

  SystemTrayIcon tray;
  EXPECT_TRUE(tray.mute_enabled());
  tray.SetMuteEnabled(false);
  EXPECT_FALSE(tray.mute_enabled());
  tray.SetMuteEnabled(true);
  EXPECT_TRUE(tray.mute_enabled());
}

TEST(TraySettingsReload, RegistersAndPresentsLikeQt) {
  EXPECT_TRUE(TraySettingsReload::ShouldReloadOnSettingsClose());
  EXPECT_TRUE(TraySettingsReload::ShowTray(true));
  EXPECT_FALSE(TraySettingsReload::ShowTray(false));
  EXPECT_TRUE(TraySettingsReload::IsRegistered(1));
  EXPECT_FALSE(TraySettingsReload::IsRegistered(0));
  EXPECT_TRUE(TraySettingsReload::ShouldRegister(true, false));
  EXPECT_FALSE(TraySettingsReload::ShouldRegister(true, true));
  EXPECT_FALSE(TraySettingsReload::ShouldRegister(false, false));
  EXPECT_TRUE(TraySettingsReload::ShouldUnregister(false, true));
  EXPECT_FALSE(TraySettingsReload::ShouldUnregister(false, false));
  EXPECT_FALSE(TraySettingsReload::ShouldUnregister(true, true));
  EXPECT_TRUE(TraySettingsReload::ShouldRefreshProgress());
  EXPECT_TRUE(TraySettingsReload::ShouldPresentWindowAfterDisable(false, false));
  EXPECT_FALSE(TraySettingsReload::ShouldPresentWindowAfterDisable(false, true));
  EXPECT_FALSE(TraySettingsReload::ShouldPresentWindowAfterDisable(true, false));
}

TEST(BusyIndicatorAnim, StartsOnShowStopsOnHide) {
  EXPECT_TRUE(BusyIndicatorAnim::ShouldStartOnShow());
  EXPECT_TRUE(BusyIndicatorAnim::ShouldStopOnHide());
}
