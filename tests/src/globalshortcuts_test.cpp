#include "globalshortcuts/globalshortcutgrab.h"
#include "globalshortcuts/globalshortcuts.h"
#include "globalshortcuts/globalshortcutsbackend-kglobalaccel.h"
#include "globalshortcuts/globalshortcutsbackend-portal.h"
#include "globalshortcuts/globalshortcutsportal.h"
#include "globalshortcuts/keymapper_x11.h"
#include "core/settings.h"
#include "playlist/playlistmanager.h"
#include "playlist/playlistsequence.h"

#include <gtest/gtest.h>

TEST(GlobalShortcuts, ShortcutIdsMatchQt) {
  const std::vector<std::string> ids = GlobalShortcutsManager::ShortcutIds();
  const std::vector<std::string> expected = {
      "play",         "pause",        "play_pause", "stop",           "stop_after",
      "next_track",   "prev_track",   "restart_or_prev_track", "inc_volume", "dec_volume",
      "mute",         "seek_forward", "seek_backward",         "show_hide",  "show_osd",
      "toggle_pretty_osd", "shuffle_mode", "repeat_mode", "toggle_scrobbling", "love"};
  ASSERT_EQ(expected.size(), ids.size());
  EXPECT_EQ(expected, ids);
}

TEST(GlobalShortcuts, CanonicalIdAliases) {
  EXPECT_EQ("play_pause", GlobalShortcutsManager::CanonicalId("playpause"));
  EXPECT_EQ("next_track", GlobalShortcutsManager::CanonicalId("next"));
  EXPECT_EQ("prev_track", GlobalShortcutsManager::CanonicalId("previous"));
  EXPECT_EQ("inc_volume", GlobalShortcutsManager::CanonicalId("volume_up"));
  EXPECT_EQ("dec_volume", GlobalShortcutsManager::CanonicalId("volume_down"));
  EXPECT_EQ("love", GlobalShortcutsManager::CanonicalId("love"));
  EXPECT_EQ("show_osd", GlobalShortcutsManager::CanonicalId("show_osd"));
}

TEST(GlobalShortcuts, FriendlyNameAndDefaultKey) {
  EXPECT_EQ("Play/Pause", GlobalShortcutsManager::FriendlyName("playpause"));
  EXPECT_EQ("MediaPlay", GlobalShortcutsManager::DefaultKey("play_pause"));
  EXPECT_EQ("MediaNext", GlobalShortcutsManager::DefaultKey("next"));
  EXPECT_EQ("", GlobalShortcutsManager::DefaultKey("show_osd"));
}

TEST(GlobalShortcuts, ShortcutByIdResolvesAlias) {
  GlobalShortcutsManager manager;
  ASSERT_NE(nullptr, manager.ShortcutById("play_pause"));
  ASSERT_NE(nullptr, manager.ShortcutById("playpause"));
  EXPECT_EQ(manager.ShortcutById("play_pause"), manager.ShortcutById("playpause"));
  EXPECT_EQ("MediaPlay", manager.ShortcutById("play_pause")->default_key());
}

TEST(GlobalShortcuts, EmitDispatchesSignals) {
  GlobalShortcutsManager manager;
  int play = 0;
  int play_pause = 0;
  int show_osd = 0;
  int love = 0;
  int seek_forward = 0;
  int activated = 0;
  manager.Play.Connect([&] { ++play; });
  manager.PlayPause.Connect([&] { ++play_pause; });
  manager.ShowOSD.Connect([&] { ++show_osd; });
  manager.Love.Connect([&] { ++love; });
  manager.SeekForward.Connect([&] { ++seek_forward; });
  manager.ShortcutById("play_pause")->Activated.Connect([&] { ++activated; });

  manager.Emit("play");
  manager.Emit("playpause");
  manager.Emit("play_pause");
  manager.Emit("show_osd");
  manager.Emit("love");
  manager.Emit("seek_forward");

  EXPECT_EQ(1, play);
  EXPECT_EQ(2, play_pause);
  EXPECT_EQ(2, activated);
  EXPECT_EQ(1, show_osd);
  EXPECT_EQ(1, love);
  EXPECT_EQ(1, seek_forward);
}

TEST(GlobalShortcuts, ReloadSettingsReadsLegacyKeys) {
  Settings settings;
  settings.BeginGroup("GlobalShortcuts");
  settings.SetBoolValue("enabled", false);
  settings.SetValue("playpause", "Ctrl+P");
  settings.SetValue("show_osd", "F12");
  settings.SetValue("next", "MediaNext");
  settings.Sync();

  GlobalShortcutsManager manager;
  manager.ReloadSettings();
  ASSERT_NE(nullptr, manager.ShortcutById("play_pause"));
  EXPECT_EQ("Ctrl+P", manager.ShortcutById("play_pause")->key());
  EXPECT_EQ("F12", manager.ShortcutById("show_osd")->key());
  EXPECT_EQ("MediaNext", manager.ShortcutById("next_track")->key());
}

TEST(GlobalShortcuts, BackendTypeNames) {
  GlobalShortcutsManager manager;
  GlobalShortcutsBackendKGlobalAccel kde(&manager);
  GlobalShortcutsBackendGnome gnome(&manager);
  GlobalShortcutsBackendPortal portal(&manager);
  EXPECT_EQ(GlobalShortcutsBackend::Type::KGlobalAccel, kde.type());
  EXPECT_EQ("KGlobalAccel", kde.name());
  EXPECT_EQ(GlobalShortcutsBackend::Type::Gnome, gnome.type());
  EXPECT_EQ("Gnome", gnome.name());
  EXPECT_EQ(GlobalShortcutsBackend::Type::Portal, portal.type());
  EXPECT_EQ("Portal", portal.name());
  EXPECT_FALSE(manager.HasActiveBackend(GlobalShortcutsBackend::Type::KGlobalAccel));
  EXPECT_FALSE(manager.HasActiveBackend(GlobalShortcutsBackend::Type::Gnome));
  EXPECT_FALSE(manager.HasActiveBackend(GlobalShortcutsBackend::Type::Portal));
}

TEST(GlobalShortcutsPortal, AcceleratorAndBindings) {
  EXPECT_EQ("XF86AudioPlay", GlobalShortcutsPortal::Accelerator("MediaPlay"));
  EXPECT_EQ("XF86AudioStop", GlobalShortcutsPortal::Accelerator("MediaStop"));
  EXPECT_EQ("XF86AudioNext", GlobalShortcutsPortal::Accelerator("MediaNext"));
  EXPECT_EQ("XF86AudioPrev", GlobalShortcutsPortal::Accelerator("MediaPrevious"));
  EXPECT_EQ("Ctrl+L", GlobalShortcutsPortal::Accelerator("Ctrl+L"));
  EXPECT_TRUE(GlobalShortcutsPortal::Accelerator("").empty());

  GlobalShortcutsManager manager;
  const auto bindings = GlobalShortcutsPortal::Bindings(manager);
  bool found_play = false;
  for (const auto &binding : bindings) {
    if (binding.first == "play_pause") {
      found_play = true;
      EXPECT_EQ("XF86AudioPlay", binding.second);
    }
    EXPECT_FALSE(binding.second.empty());
  }
  EXPECT_TRUE(found_play);
}

TEST(KeyMapperX11, QtShortcutToKey) {
  EXPECT_EQ(QtKey::MediaPlay, KeyMapperX11::QtShortcutToKey("MediaPlay"));
  EXPECT_EQ(QtKey::MediaNext, KeyMapperX11::QtShortcutToKey("Media Next"));
  EXPECT_EQ(QtKey::MediaPrevious, KeyMapperX11::QtShortcutToKey("MediaPrevious"));
  EXPECT_EQ(QtKey::MediaStop, KeyMapperX11::QtShortcutToKey("MediaStop"));
  EXPECT_EQ(QtKey::VolumeUp, KeyMapperX11::QtShortcutToKey("VolumeUp"));
  EXPECT_EQ(QtKey::VolumeDown, KeyMapperX11::QtShortcutToKey("VolumeDown"));
  EXPECT_EQ(QtKey::VolumeMute, KeyMapperX11::QtShortcutToKey("Mute"));
  EXPECT_EQ(QtKey::Space, KeyMapperX11::QtShortcutToKey("Space"));
  EXPECT_EQ(QtKey::F1 + 11u, KeyMapperX11::QtShortcutToKey("F12"));
  EXPECT_EQ(QtKey::ControlModifier | static_cast<unsigned>('P'), KeyMapperX11::QtShortcutToKey("Ctrl+P"));
  EXPECT_EQ(QtKey::ControlModifier | QtKey::Space, KeyMapperX11::QtShortcutToKey("<Primary>space"));
  EXPECT_EQ(0u, KeyMapperX11::QtShortcutToKey(""));
  EXPECT_TRUE(KeyMapperX11::IsMediaKeyName("MediaPlay"));
  EXPECT_FALSE(KeyMapperX11::IsMediaKeyName("Ctrl+P"));
}

TEST(GlobalShortcutGrab, AcceptsCompleteCombosOnly) {
  EXPECT_STREQ("Press a key", GlobalShortcutGrab::WindowTitle());
  EXPECT_STREQ("Waiting…", GlobalShortcutGrab::WaitingLabel());
  EXPECT_EQ("Press a key combination.", GlobalShortcutGrab::Prompt(""));
  EXPECT_EQ("Press a key combination to use for Play/Pause...", GlobalShortcutGrab::Prompt("Play/Pause"));
  EXPECT_TRUE(GlobalShortcutGrab::IsModifier(GDK_KEY_Control_L));
  EXPECT_TRUE(GlobalShortcutGrab::IsModifier(GDK_KEY_Shift_R));
  EXPECT_TRUE(GlobalShortcutGrab::IsModifier(GDK_KEY_Alt_L));
  EXPECT_TRUE(GlobalShortcutGrab::IsModifier(GDK_KEY_ISO_Level3_Shift));
  EXPECT_FALSE(GlobalShortcutGrab::IsModifier(GDK_KEY_p));
  EXPECT_FALSE(GlobalShortcutGrab::ShouldAccept(GDK_KEY_Control_L));
  EXPECT_TRUE(GlobalShortcutGrab::ShouldAccept(GDK_KEY_space));
  EXPECT_EQ("<b>Ctrl+P</b>", GlobalShortcutGrab::PreviewMarkup("Ctrl+P"));
  EXPECT_TRUE(GlobalShortcutGrab::PreviewMarkup("").empty());
  EXPECT_TRUE(GlobalShortcutGrab::RejectClears(""));
  EXPECT_FALSE(GlobalShortcutGrab::RejectClears("<b>Ctrl</b>"));
}

TEST(PlaylistManager, CycleRepeatAndShuffleModes) {
  PlaylistManager manager(nullptr, nullptr, nullptr, nullptr, nullptr);
  manager.Init();
  ASSERT_NE(nullptr, manager.current());
  EXPECT_EQ(PlaylistSequence::RepeatMode::Off, manager.current()->repeat_mode());
  manager.CycleRepeatMode();
  EXPECT_EQ(PlaylistSequence::RepeatMode::Track, manager.current()->repeat_mode());
  manager.CycleShuffleMode();
  EXPECT_EQ(PlaylistSequence::ShuffleMode::All, manager.current()->shuffle_mode());
}
